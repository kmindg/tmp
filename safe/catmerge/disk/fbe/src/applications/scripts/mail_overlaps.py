import smtplib
import urllib2
import re
import subprocess
from email.mime.text import MIMEText
from email.mime.multipart import MIMEMultipart
import datetime
import os
import sys
import traceback

STREAMS     = ['upc-nextperformance-cs', 'upc-mcxbugfix-cs']
MAIL_SERVER = "mailhub.lss.emc.com"
SENDER      = "robert.foley@emc.com" #"USD.CE.FBE.Test.Nightly.Results@emc.com"
RECIPIENTS   = ["robert.foley@emc.com","peter.puhov@emc.com","kimchi.mai@emc.com"]
COMMASPACE = ', '

HISTORY_STREAMS = ['upc-performance-cs', 'upc-mcxbugfix-cs', 'upc-nextperformance-cs', 'upc-perf-unity-cs']
HISTORY_RECIPIENTS = ['USD.CE.FBE.Test.Nightly.Results@emc.com']
ADMIN_EMAIL = ["robert.foley@emc.com"]
#accurev hist -s upc-nextperformance-cs -t "2014/11/11 06:00:00-now"

def get_date_time():
    now = datetime.datetime.now()
    current_date_time = now.strftime('%m/%d/%Y %H:%M:%S')
    return current_date_time

class writer(object):
    def __init__(self, *files):
        self.files = list(files)
        #print self.files

    def add(self, f):
        self.files.append(f)

    def write(self, txt):
        for fp in self.files:
            fp.write(txt)
            fp.flush()
    def flush(self):
        for fp in self.files:
            fp.flush()


def setup_log_file():
    f = open("c:\mail_results.txt", "w")
    global log_object
    log_object = writer(f, sys.stdout)
    sys.stdout = log_object
    sys.stderr = log_object

def synctime():
    #cmd = ['accurev' , 'synctime']
    #proc = subprocess.Popen(cmd, shell=True, stdout=subprocess.PIPE)
    #output = proc.communicate()[0];
    status = os.system("accurev synctime")
    print "Synctime output: %s" % status 
    return status

def get_overlaps(stream):

    cmd = ['accurev' , 'stat', '-s', stream, '-B', '-o', '-fn']
    proc = subprocess.Popen(cmd, shell=True, stdout=subprocess.PIPE)
    overlap_content = proc.communicate()[0]; 
    return overlap_content

def mail_overlaps():
    for stream in STREAMS:
        print "Checking stream: %s" % stream
        subject = "deep overlaps found for stream: " +  stream
        overlaps = get_overlaps(stream)
        #print overlaps
        content =""
        lines = overlaps.split("\n")
        for line in lines:
            if "catmerge" not in line:
                continue
            content = content + line + '\n'
        print content
        msg = MIMEMultipart()
        if content == "":
            print "no deep overlaps found for stream: %s \n" % stream
        else:
            msg['Subject']  = subject
            msg['From']     = SENDER
            msg['To']       = COMMASPACE.join(RECIPIENTS)


            html = MIMEText(content, 'html')
        
            msg.attach(html)

            smtp = smtplib.SMTP(MAIL_SERVER)
            smtp.sendmail(SENDER, RECIPIENTS, msg.as_string())
            print "sent mail for stream stream: %s \n" % (stream)
            smtp.quit()
class stream_history:
    def __init__(self):
        self.day = 0

    def get_history(self, stream, days):

        now = datetime.datetime.now()
        today = datetime.date.today()
        yesterday = today - datetime.timedelta(days)
        yesterday = yesterday.strftime('%Y/%m/%d 06:00:00')
        today = today.strftime('%Y/%m/%d 06:00:00')
        time_range = '-t ' + yesterday + '-' + today
        cmd = ['accurev', 'hist', '-s', stream, time_range]
        #print cmd
        proc = subprocess.Popen(cmd, shell=True, stdout=subprocess.PIPE)
        history = proc.communicate()[0]; 
        #print "[" + history
        if "No history corresponding to selection." in history:
            return ""
        return history

    def mail_stream_history(self, days, recipient):
        if days == 7:
            subject = "Weekly Summary of MCR Stream History"
        elif days == 1:
            subject = "Daily MCR Stream History"
        else:
            subject = "Summary of MCR Stream History for Past %u Days" % days

        overall_content = ""
        not_found_content = ""
        for stream in HISTORY_STREAMS:
            print "stream: %s"  % stream
            history = self.get_history(stream, days)
            if history == "":
                not_found_content += "    <H2>%s</H2>\n" % stream
            else:
                overall_content += "<H1>Stream: %s </H1>" % stream
                #print overlaps
                history = history.replace('\r','')
                lines = history.split("\n")
                content = ""
                index = 0
                for line in lines:
                    line = line.replace('\n','')
                    if line != "":
                        if "transaction" and "user:" in line and index != 0:
                            content += '<BR><BR>\n\n' + line + '<BR>\n'
                        else:
                            content += line + '<BR>\n'
                        index = index + 1
                overall_content = overall_content + content
                overall_content += "<BR>\n"

        if not_found_content:
            overall_content += "<H1>Streams with no new promotions since yesterday.</H1>\n"
            overall_content += not_found_content + "<BR>\n"

        print overall_content
    
        msg = MIMEMultipart()
        if overall_content == "":
            print "no content found for stream: %s " % stream
        else:
            msg['Subject']  = subject
            msg['From']     = SENDER
            msg['To']       = COMMASPACE.join(recipient)
            
            html = MIMEText(overall_content, 'html')
            
            msg.attach(html)
            
            smtp = smtplib.SMTP(MAIL_SERVER)
            smtp.sendmail(SENDER, recipient, msg.as_string())
            print "sent stream history mail for %u days summary\n" % days
            smtp.quit()

def main():
    setup_log_file()
    print "%s starting" % get_date_time()
    synctime()
    force = False
    sh = stream_history()
    today = datetime.date.today()
    email = HISTORY_RECIPIENTS 
    #email = ADMIN_EMAIL

    # Daily report.
    sh.mail_stream_history(1, email)
                                 
    if force or today.weekday() == 0:
        # Monday.  Send weekly report.
        sh.mail_stream_history(7, email)

    if force or today.day == 1:
        # First of month, send monthy report.
        sh.mail_stream_history(30, email)    

    mail_overlaps()

    print "%s done" % get_date_time()

if __name__ == "__main__":

    orig_out = sys.stdout
    orig_err = sys.stderr

    try:
        main()
    except KeyboardInterrupt:
        print "*****************************"
        print "** Got CTRL-C; Shutting down"
        print "*****************************"
        exit(1)
    except Exception as e:
        traceback.print_exc()
        print e
    finally:
        sys.stdout = orig_out
        sys.stderr = orig_err


