KidMon Overview
===============

KidMon collects data about running applications to get info what apps are used and how long.
Only application in foreground is considered and human interraction (mouse, keyboard) is taken in account.
Statistics are written to CSV hourly for each process and memory released. So it should not mem leak.
So may be running for every logged-in user but only once instance per particular user.
Directories are automatically created when do not exist.
When -c command line switch is used then program runs without console, just visible in Task Manager at process tab.
Process can be killed from there but data remain unsaved from the last hour as it is SIGKILL signal.
The program handles WM_ENDSESSION commands so it should flush data when computer is shutting down.

It is supposed the program is started when user is logged in.

Installation
------------
Copy executable to any location, e.g. "c:\Program Files (x86)\KidMon\" and satisfy running "kidmon.exe -c"
whenever needed. Ideally start it when any user logs in.

Use gpedit.msc 
Computer Configuration\Administrative Templates\System\Login\Run These Programs at User Logon

It is related to key in:
HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows\CurrentVersion\Policies\Explorer\Run 
HKEY_LOCAL_MACHINE\SOFTWARE\Wow6432Node\Microsoft\Windows\CurrentVersion\Policies\Explorer\Run
and seems at user level:
HKEY_CURRENT_USER\Software\Microsoft\Windows\CurrentVersion\Group Policy Objects\{0463233C-423F-4708-8E72-46B0A592106D}Machine\Software\Microsoft\Windows\CurrentVersion\Policies\Explorer\Run

Also creating shortcut in 
C:\ProgramData\Microsoft\Windows\Start Menu\Programs\Startup
or 
C:\Users\<user-name>\AppData\Roaming\Microsoft\Windows\Start Menu\Programs\Startup
will do job as well but it is visible in Start menu.

powershell "$s=(New-Object -COM WScript.Shell).CreateShortcut('%programdata%\Microdoft\Windows\Start Menu\Programs\Startup\KidMon.lnk');$s.TargetPath='<path>\warp-cli.exe';$s.Arguments='-c';$s.WindowStyle=7;$s.Save()"
see https://stackoverflow.com/questions/30028709/how-do-i-create-a-shortcut-via-command-line-in-windows

Or add scheduled "at logon" job for any user. But it seems requires account used for program being executed. It is useless for global settings.

schtasks.exe /Create /SC ONLOGON /TN KidMon /TR "'<path>\KidMon.exe' -c"

Output file
-----------

Default output filename is "%UserProfile%\AppData\LocalLow\MandySoft\KidMon\KidMon_hourly_raw.csv.

The program is appending rows every round hour. There should be a process which collects data and
passes to e.g. RRD-style database for days, weeks, years. So the data size won't grow infinitely. When done the
program may delete just processed data from output file.

CSV file columns are:

timestamp;appname;time spent without human interraction in ms;time spent with human interraction in ms

Example:
2021-05-18 23:00;"\Device\HarddiskVolume2\Program Files (x86)\Google\Chrome\Application\chrome.exe";0;5000
2021-05-18 23:00;"\Device\HarddiskVolume2\Windows\System32\taskmgr.exe";0;20000
2021-05-18 23:00;"\Device\HarddiskVolume2\Windows\explorer.exe";0;10000
2021-05-18 23:00;"\Device\HarddiskVolume2\Windows\System32\cmd.exe";0;15000
2021-05-19 19:00;"\Device\HarddiskVolume2\Windows\System32\cmd.exe";50000;10000
2021-05-19 19:00;"\Device\HarddiskVolume2\Windows\System32\taskmgr.exe";0;5000

Requirements
------------
OS: Windows 7, 10
Visual C++ Redistributable for Visual Studio 2015 (MSVCP140.dll, VCRUNTIME140.dll)

Note:
The functionality cannot be implemented as (system) service as API function cannot provide information about
user applications (active window, etc.). So the workaround is hidden application under user account.
