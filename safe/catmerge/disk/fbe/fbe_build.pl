#!/usr/bin/perl
package main;

use FindBin;
use lib "$FindBin::Bin/../../../../build";
use Cwd;
use File::Basename;
use Rpm;
use File::Find;

use strict;


my $current_dir = getcwd();
my $safe_dir = 'safe/catmerge/disk/fbe';
my $workspace_dir = $current_dir;
$workspace_dir =~ s/$safe_dir//ig;
print "workspace: ".$workspace_dir."\n";
chdir($workspace_dir);
my $base_rpmlist = '/src/rpmlist';
#my $map_rpmlist = '/src/map_rpmlist';
my $rpm_repository = "";


sub print_usage
{
	print "\nUsage: fbe_build.pl [build_all components] [-r|--rpm_repository <path_to_workspace_output_dir>] [build_all options]\n";
	print "\nExample: fbe_build.pl safe -r /c4_working/views/HengS1/snickers/output --args safe=\"--subdir=/c4_working/views/HengS1/twix/safe/catmerge/disk/fbe\" --nosetupwspace\n";
	print "\nExample: fbe_build.pl safe image upgrade -r /c4_working/views/HengS1/snickers/output --nosetupwspace\n";
}

sub process_rules
{
	my $file = "tmplsrules";
	system("ac lsrules > $file");
	open(FILE, "<tmplsrules");
	my $output = "";
	while (my $line = <FILE>)
	{
		$line =~ s/^excl\s(.*)/$1/ig;
		$line =~ s/^incldo\s(.*)/$1/ig;
		$line =~ s/^incl\s(.*)//ig;
		$output .= $line;
	}


	close(FILE);
	open(FILE, ">$file");
	print FILE $output;

	close(FILE);

	system("ac clear -l $file");
	system("rm $file");
}

sub setup_thirdparty
{
	my $thirdparty_dir = $workspace_dir."thirdparty";
	print "thirdparty_dir is $thirdparty_dir\n";
	
	return if ($rpm_repository eq "" || -d $thirdparty_dir );

	my $target = $rpm_repository;
	$target =~ s/output/thirdparty/ig;
	print "thirdparty target is $target\n";
	my $cmd = "ln -s $target $thirdparty_dir";
        print "Creating symbolic link: $cmd\n";
        system($cmd); 
}

sub process_repo_links
{
	#my $file = "build/repoLinks.txt";
    my @files = ("build/repoLinks.txt", "build/fileLinks.txt");
    foreach my $file (@files) {

	open(FILE, "<$file");

	while (my $line = <FILE>)
	{
		if ($line =~ /(.*)\s(\"safe\/.*)/)
		{
			my $target = $1;
			my $destination = $2;
		
			my $temp = $destination;
			$temp =~ s/\"//ig;
			my ($file, $dir, $ext) = fileparse($temp);
			if (!(-d $dir))
			{
		    		print "creating dir $dir\n";
				system("mkdir -p $dir");
			} 
		        
                        system("unlink $destination");
			my $cmd = "ln -s $target $destination";
			print "Creating symbolic link: $cmd\n";
			system($cmd);
		}
		elsif ($line =~ /\"(.*)\"\s\"(.*)\"/)
		{
			my $target = $1;
			my $destination = $2;
			$target =~ s/\"//ig;
			$destination =~ s/\"//ig;
			my ($file, $dir, $ext) = fileparse($destination);
                        if (!(-d $dir))
                        {
                                print "creating dir $dir\n";
                                system("mkdir -p $dir");
                        }

			my $temp = "unlink $destination";
			print "$temp\n";
                        system($temp);
                        my $cmd = "ln -s $target $destination";
                        print "Creating symbolic link: $cmd\n";
                        system($cmd);
			

		}
	}

	close(FILE);
    }	
}

sub clean_safe_output_dir
{
	my $safe_output_dir = $workspace_dir."output/safe";
	print "Cleaning safe output directory $safe_output_dir\n";
	system("rm -rf $safe_output_dir");

}

#-------------------------------------------------------------
#-- Parameters:
#--   @paths: list of directories to search for RPM files
#--
#-- Looks in each given path and its subdirs for rpm files
#-- and returns a list of all the rpms found
#-------------------------------------------------------------
sub rpms_in_path {
  my (@paths) = @_;
  my @rpms;

  foreach my $path (@paths) {
    my $abs_path = Cwd::abs_path($path);
    die "ERROR: Cannot resolve path $path" unless defined $abs_path;
    find( sub { push @rpms, Rpm->new($File::Find::name) if $File::Find::name =~ /\.(?:rpm|tar|tgz|jar)$/ }, $abs_path );
  }

  return @rpms;
}


# process the command line arguments
my @tmp_args = @ARGV;
my @args = ();
my $index = 0;
my $is_comp_done = 0;
my @components = ();
while (my $arg = shift(@tmp_args)) {
	$arg =~ s/(safe\=)(.*)/$1\"$2\"/ig;
	
	if ($arg =~ /\-/) {
		$is_comp_done = 1;
	}

	if ($arg eq '-r' || $arg eq '--rpm_repository') {
		$rpm_repository = shift(@tmp_args);
	} elsif ($arg eq '-h' || $arg eq '--help') {
		print_usage();
		exit(0);
	} elsif ($arg !~ /\-/ && !$is_comp_done) {
		# this is a component
		push @args, $arg;
		$arg =~ s/\+|\-//ig;
		push @components, $arg;
	
	} else {
		push @args, $arg;
	}
	$index++;
}

process_rules();
setup_thirdparty();
process_repo_links();

my $args_list = join " ", @args;
my $build_all_cmd = "build_all $args_list";
my %map_rpm_hash = ();

if ($rpm_repository ne "") {
	my @rpms_in_repo = rpms_in_path($rpm_repository);
        print "Found ".scalar(@rpms_in_repo)." rpms in $rpm_repository\n";
        my @rpms_in_ws = rpms_in_path($workspace_dir."output");
        print "Found ".scalar(@rpms_in_ws)." rpms in ".$workspace_dir."output\n";
	clean_safe_output_dir();
	foreach my $component (@components) {
		#next if ($component eq "upgrade");
		@{$map_rpm_hash{$component}} = ();

		if ($component eq "upgrade")
		{
			my $base_image_file = $workspace_dir."image/src/baseimage";
			my $base_image_file_temp = $base_image_file.".temp";
			my $base_image_repository = $rpm_repository."/image";#{TARGET}_{FLAVOR}';
			print "base_image_folder: $base_image_repository\n";
			opendir(IMAGEDIR, $base_image_repository)|| next;
			my @subdirs = readdir(IMAGEDIR);
			my $base_image = "";
			foreach my $subdir (@subdirs) {
				$subdir = $base_image_repository."/".$subdir;
				if (-d $subdir && $subdir !~ /\./ && $base_image eq "") {
					opendir(TARGETDIR, $subdir)|| die "unable to open $subdir\n";
					my @files = readdir(TARGETDIR);
					foreach my $file (@files) {
						if ($file =~ /\.tgz\.bin/) {
							$base_image = $file;		
						}
					}
					closedir(TARGETDIR);
				}
			}
			closedir(IMAGEDIR);
			if ($base_image ne "") {
				$base_image =~ s/(.*\-)(\w+)\_(\w+)(\.tgz\.bin)/$1\{TARGET\}\_\{FLAVOR\}$4/ig;
				my $base_image_path = $base_image_repository."/{TARGET}_{FLAVOR}/".$base_image;
				print "Upgrade base image is $base_image_path\n";
				system("cp $base_image_file $base_image_file_temp");
				push @{$map_rpm_hash{$component}}, $base_image_file;
				open(BASEIMAGEFILE, ">$base_image_file");
				print BASEIMAGEFILE $base_image_path;
				close(BASEIMAGEFILE);
			}
			#next;
                        $component = "image";
		}
		
		my $rpmlist = $component.$base_rpmlist;
		my @rpmlists = ($rpmlist);
		if (grep {$_ eq $component} qw(boothflash image imager))
		{
			@rpmlists = ($rpmlist."_emc", $rpmlist."_opensrc", $rpmlist."_opensrc_v3" );
		}
				
		foreach my $rpmlist (@rpmlists) {
			my $map_rpmlist = $rpmlist;
			$map_rpmlist =~ s/(.*\/)rpmlist(.*)/$1map_rpmlist$2/ig;
			push @{$map_rpm_hash{$component}}, $map_rpmlist;
			my $temp_map_rpmlist = $map_rpmlist.".temp";
			print "rpm map-- $rpmlist $map_rpmlist $temp_map_rpmlist\n";
				
			system("cp $map_rpmlist $temp_map_rpmlist");
			system("$workspace_dir/build/update_rpmlist --rpmlist $rpmlist --path $rpm_repository");
		}
		
	}
}

print "Build Command: $build_all_cmd\n";
system($build_all_cmd);

if ($rpm_repository ne "")
{
    foreach my $component (@components)
    {
	next if ($component eq "upgrade");
	my @map_rpmlists = @{$map_rpm_hash{$component}};
	foreach my $map_rpmlist (@map_rpmlists) { 
		my $temp_map_rpmlist = $map_rpmlist.".temp";
		#print "$temp_map_rpmlist $map_rpmlist\n";
		system("mv $temp_map_rpmlist $map_rpmlist");
		
	}
    }
}  


1;




