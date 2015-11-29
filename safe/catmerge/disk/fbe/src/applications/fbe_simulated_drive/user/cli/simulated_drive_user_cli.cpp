#include <cstdlib>
#include <cstring>
#include <iostream>
#include <iomanip>
#include <string>
#include <vector>
#include <map>

#include "simulated_drive_private_cpp.h"

#include "fbe/fbe_emcutil_shell_include.h"

using namespace std;

#define MAX_SERVER_NAME 64
#define MAX_COMMAND 64
#define DEFAULT_DISPLAY_UNIT 64

vector<string> split_string(const char *str, const char *splitter);

// ----------begin of class cli_command-----------
class cli_command
{
public:

    cli_command(const char *name, const char *description, const char *cmd, const char *splitter, const char *options, int least_options, int (*handler)(vector<string>& cmds));
    simulated_drive_bool_t accept(const char *cmd);
    int handle(const char *cmd);
    void print_introduction(void);

private:
    string _name;
    string _description;
    string _options;
    int _least_options;
    int (*_handler)(vector<string>& cmds);
    map<string, string> commands;
};

cli_command::cli_command(const char *name, const char *description, const char *cmd, const char *splitter, const char *options, int least_options, int (*handler)(vector<string>& cmds))
:_name(name), _description(description), _options(options), _least_options(least_options), _handler(handler)
{
    vector<string> cmds = split_string(cmd, splitter);
    for (int i = 0; i < (int)cmds.size(); ++i)
    {
        size_t start, end;
        if ((start = cmds[i].find_first_of('(')) == string::npos)
        {
            commands[cmds[i]] = cmds[i];
        }
        else
        {
            end = cmds[i].find_first_of(')');
            if (end == string::npos)
            {
                end = cmds[i].size();
            }
            commands[cmds[i].substr(0, start)] = cmds[i].substr(start + 1, end - start - 1);
        }
    }
}

simulated_drive_bool_t cli_command::accept(const char *cmd)
{
    vector<string> cmds = split_string(cmd, " ");
    if (cmds.size() > 0)
    {
        for (map<string, string>::iterator it = commands.begin(); it != commands.end(); ++it)
        {
            if (!cmds[0].compare((*it).first) || !cmds[0].compare((*it).second))
            {
                return TRUE;
            }
        }
    }
    return FALSE;
}

int cli_command::handle(const char *cmd)
{
    vector<string> cmds = split_string(cmd, " ");
    if (cmds.size() < 1)
    {
        return -1;
    }
    cmds.erase(cmds.begin());
    if ((int)cmds.size() < _least_options)
    {
        cout << "Insufficient arguments." << endl;
        return 0;
    }
    return _handler(cmds);
}

void cli_command::print_introduction(void)
{
    simulated_drive_bool_t first = TRUE;
    cout << _description << endl;
    cout << "\t";
    for (map<string, string>::iterator it = commands.begin(); it != commands.end(); ++it)
    {
        if (first)
        {
            first = FALSE;
        }
        else
        {
            cout << "/";
        }
        cout << (*it).first;
        if ((*it).first.compare((*it).second))
        {
            cout << "(" << (*it).second << ")";
        }
    }
    cout << " " << _options << endl;
}
// ----------end of class cli_command-----------

vector<string> split_string(const char *str, const char *splitter)
{
    vector<string> result;
    char *temp = new char[strlen(str) + 1];
    strcpy(temp, str);

    char * pch = strtok (temp,splitter);
    while (pch != NULL)
    {
        result.push_back(pch);
        pch = strtok (NULL, splitter);
    }
    return result;
}

void segment_print(simulated_drive_byte_t *buffer, simulated_drive_size_t size, simulated_drive_size_t unit, simulated_drive_size_t begin)
{
    simulated_drive_size_t each, cnt = size;
    if (unit <= 0)
    {
        unit = size;
    }
    cout << size << " bytes read:" << endl;
    while (cnt > 0)
    {
        each = min(unit, cnt);
        cout << "[" << begin << " - " << begin + each -1 << "]" << endl;
        for(int i = 0; i < each; ++i)
        {
            cout << "0x" << setw(2) << setfill('0') << hex << (int)(unsigned char)buffer[size - cnt + i] << " ";
        }
        cout << dec << endl;
        cnt -= each;
        begin += each;
    }
}


vector<cli_command> commands;

// ------------begin of command handlers-------------
int handle_drive(vector<string>& cmds)
{
    simulated_drive_list_t raw_drives = simulated_drive_get_drive_list();
    vector<simulated_drive_t> drives;
    if (cmds.size() < 1)
    {
        for(int i = 0; i < raw_drives.size; ++i)
        {
            drives.push_back(raw_drives.drives[i]);
        }
    }
    else
    {
        for (int i = 0; i < (int)cmds.size(); ++i)
        {
            for (int j = 0; j < raw_drives.size; ++j)
            {
                if (!strncmp(cmds[i].c_str(), raw_drives.drives[j].drive_identity, sizeof(simulated_drive_identity_t)))
                {
                    int k;
                    for (k = 0; k < (int)drives.size(); ++k)
                    {
                        if (!strncmp(cmds[i].c_str(), drives[k].drive_identity, sizeof(simulated_drive_identity_t)))
                        {
                            break;
                        }
                    }
                    if (k >= (int)drives.size())
                    {
                        drives.push_back(raw_drives.drives[j]);
                        break;
                    }
                }
            }
        }
    }
    cout << drives.size() << " drive(s):" << endl;
    if (drives.size() > 0)
    {
        cout << "IDENTITY\tBLOCK SIZE\tMAX LBA" << endl;
        for (int i = 0; i < (int)drives.size(); ++i)
        {
            cout << drives[i].drive_identity << "\t\t" << drives[i].drive_block_size << "\t\t" << drives[i].max_lba << endl;
        }
    }
    return 0;
}

int handle_quit(vector<string>& cmds)
{
    return 1;
}

int handle_help(vector<string>& cmds)
{
    for (int i = 0; i < (int)commands.size(); ++i)
    {
        commands[i].print_introduction();
    }
    return 0;
}

int handle_read(vector<string>& cmds)
{
    simulated_drive_identity_t identity;
    memset(identity, '\0', sizeof(simulated_drive_identity_t));
    memcpy(identity, cmds[0].c_str(), sizeof(simulated_drive_identity_t));
    simulated_drive_lba_t begin = atoi(cmds[1].c_str());
    simulated_drive_size_t nBytes = atoi(cmds[2].c_str()), block_size = 1;
    int unit = cmds.size() > 3 ? atoi(cmds[3].c_str()) : DEFAULT_DISPLAY_UNIT;
    simulated_drive_list_t drives = simulated_drive_get_drive_list();
    for (int i = 0; i < drives.size; ++i)
    {
        if (!strncmp(identity, drives.drives[i].drive_identity, sizeof(simulated_drive_identity_t)))
        {
            block_size = drives.drives[i].drive_block_size;
            break;
        }
        cout << "Invalid drive block size." << endl;
        return 0;
    }
    simulated_drive_handle_t handle = simulated_drive_open(identity);
    if (SIMULATED_DRIVE_INVALID_HANDLE == handle)
    {
        cout << "Invalid drive identity." << endl;
        return 0;
    }
    simulated_drive_byte_t *buffer = new simulated_drive_byte_t[(unsigned int)nBytes];
    simulated_drive_size_t nBytes_read = simulated_drive_read(handle, buffer, begin, nBytes);
    if (SIMULATED_DRIVE_INVALID_SIZE == nBytes_read)
    {
        cout << "Read error!" << endl;
        delete buffer;
        return 0;
    }
    segment_print(buffer, nBytes_read, unit, begin * block_size);
    delete buffer;
    return 0;
}
// ------------end of command handlers-------------

int __cdecl main(int argc, char **argv)
{
    char server_name[MAX_SERVER_NAME];
    simulated_drive_port_t port = SIMULATED_DRIVE_DEFAULT_PORT_NUMBER;
    memset(server_name, 0, MAX_SERVER_NAME);
    simulated_drive_bool_t help = FALSE, invalid = FALSE;
    int i;
    
#include "fbe/fbe_emcutil_shell_maincode.h"

    strcpy(server_name, SIMULATED_DRIVE_DEFAULT_SERVER_NAME);

    cli_command drive_command("drive", "Get drive infomation (all drives if no identity is specified):", "drive(d)", "/", "[identity 0] [identity 1] ...", 0, handle_drive);
    cli_command read_command("read", "Read drive content:", "read(r)", "/", "<drive identity> <lba> <bytes> [display unit]", 3, handle_read);
    cli_command quit_command("quit", "Quit the tool:", "quit(q)/exit(e)", "/", "", 0, handle_quit);
    cli_command help_command("help", "Get help infomation:", "help(h)", "/", "", 0, handle_help);

    commands.push_back(drive_command);
    commands.push_back(read_command);
    commands.push_back(quit_command);
    commands.push_back(help_command);

    for (i = 1; i < argc; ++i)
    {
        if (strlen(argv[i]) < 2 || '/' != argv[i][0] || help || invalid)
        {
            break;
        }
        switch (argv[i][1])
        {
        case('h'):
        case('H'):
            help = TRUE;
            break;
        case('s'):
        case('S'):
            strcpy(server_name, argv[i] + 2);
            break;
        case('p'):
        case('P'):
            port = atoi(argv[i] + 2);
            break;
        default:
            invalid = TRUE;
            break;
        }
    }
    if (i < argc || invalid)
    {
        cout << "Invalid command." << endl;
        return 1;
    }
    if (help)
    {
        cout << "Optional Arguments (NOTICE: NO space between argument flag and value!):" << endl;
        cout << "[/h /H] get help information" << endl;
        cout << "[/s /S] specify the server name" << endl;
        cout << "[/p /P] specify the port" << endl;
    }
    else
    {
        if (!fbe_simulated_drive_init(server_name, port, 0))
        {
            cout << "Simulated drive init error!" << endl;
            return 1;
        }
        char cmd[MAX_COMMAND];
        memset(cmd, 0, MAX_COMMAND);
        while (1)
        {
            cout << '>';
            int res = -1;
            gets(cmd);

            if(strlen(cmd) <= 0)
            {
                continue;
            }
            for (int i = 0; i < (int)commands.size(); ++i)
            {
                if (commands[i].accept(cmd))
                {
                    res = commands[i].handle(cmd);
                    break;
                }
            }

            if (res == 0)
            {
                continue;
            }
            else if (res == 1)
            {
                break;
            }
            else
            {
                cout << "Invalid command." << endl;
                continue;
            }
        }
        if (!simulated_drive_cleanup())
        {
            cout << "Simulated drive cleanup error!" << endl;
            return 1;
        }
    }
    return 0;
}


