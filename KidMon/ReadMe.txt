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

Default output filename is "%UserProfile%\AppData\LocalLow\MandySoft\KidMon\kidmon_hourly_raw.csv.

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

KidMonDude
----------

KidMon output file kidmon_hourly_raw.csv should be periodically processed using KidMonDude which aggregates data in RRD manner. 
The file kidmon_hourly_raw.csv is renamed to avoid race with KindMon which will create new file. The data are also propagated
to HTML template to visualize using Google Charts API. The CSV data are injected into local file based on template.
The charts are rendered locally on client machine, no data are sent outside.

---- kidmon_rules.csv

As the application name is logged in file as executable file name with full path, it can be translated to human application name using
rule set. It is defined in extra CSV file with format. Default location is in directory where is KidMonDude.exe.
The admin should check what application names appear in kidmon_hourly_raw.csv after a testing period and adjust kidmon_rules.csv to
get satisfying data. Typically name may contain version which changes after upgrade, application uses multiple filenames, etc.

Example:
# this is comment
# command;options;regex;command specific param
# match regex and replace whole appname
S;;FiveM;FiveM
S;;Fortnite;Fortnite
S;I;RUSSIAPHOBIA;RussiaPhobia
S;;FACEIT;FaceIt
S;;lunarclient;Lunar Client
# get basename and continue next rule
B;C;.*;0
# replace (i.e. remove) extension case-insensitive
R;I;\.exe$;

Command:
S .. search regex again name and replace whole string
	Param: string to be assigned as new name when regex matches original name
	Example:
		# when 'FACEIT' is matched in name then set name 'FaceIt' and stop rule processing
		S;;FACEIT;FaceIt

R .. search regex again name and replace particular part string
	Param: string to be replaced in name according regex
	Example:
		# replace extension case insensitive
		R;I;\.exe$;

B .. string path
	Param:
		0 .. leave only base name
		n .. leave also n dirnames preceeding basename
		-n .. strip n leftmost dirnames (and leave basename)
	Example:
		# "\Device\HarddiskVolume2\Program Files (x86)\Google\Chrome\Application\chrome.exe" -> chrome.exe
		B;C;.*;0
		# "\Device\HarddiskVolume2\Program Files (x86)\Google\Chrome\Application\chrome.exe" -> Chrome\Application\chrome.exe
		B;C;.*;2
		# "\Device\HarddiskVolume2\Program Files (x86)\Google\Chrome\Application\chrome.exe" -> "HarddiskVolume2\Program Files (x86)\Google\Chrome\Application\chrome.exe"
		B;C;.*;-1


Options:
I .. regex is case insensitive
C .. continue with next rule, i.e. do not stop processing when name matches regex
< .. apply on source file, default even not present
> .. apply to output file, i.e. rename existing names in output CSV

Regex:
see https://www.cplusplus.com/reference/regex/ECMAScript/

---- RRD files

The most recent data are available in hourly interval, less recent data in daily, weekly and monthly interval. The data format is again CSV. The time is in hours.
The output files are located in the same directory as kidmon_hourly_raw.csv.

Example:

kidmon_hourly.csv
"2021-05-27 15:00";chrome;0.790;3.630
"2021-05-27 15:00";WhatsApp;0.000;0.150
"2021-05-27 15:00";Discord;0.000;0.060
"2021-05-27 15:00";Taskmgr;0.000;0.060
"2021-05-27 15:00";FiveM;0.000;0.040
"2021-05-27 15:00";explorer;0.000;0.040
"2021-05-27 15:00";FaceIt;0.000;0.010
"2021-05-27 18:00";csgo;0.190;5.110
"2021-05-27 18:00";Discord;0.000;0.600
"2021-05-27 18:00";chrome;0.000;0.240

kidmon_daily.csv
2021-05-28;chrome;2.780;6.378
2021-05-28;Discord;0.110;2.490
2021-05-28;steam;0.000;0.430
2021-05-28;FiveM;0.000;0.420
2021-05-28;explorer;0.010;0.200
2021-05-28;EpicGamesLauncher;0.000;0.080
2021-05-28;steamwebhelper;0.000;0.010

kidmon_weekly.csv
2021-W21;chrome;6.218;11.514
2021-W21;FiveM;0.512;12.374
2021-W21;csgo;0.216;5.875
2021-W21;Discord;0.408;4.828
2021-W21;explorer;0.042;0.988
2021-W21;steam;0.000;0.288
2021-W21;NordVPN;0.070;0.058
2021-W21;notepad;0.000;0.058
2021-W21;EpicGamesLauncher;0.000;0.048
2021-W21;FaceIt;0.000;0.048

kidmon_monthly.csv
2021-05;FiveM;2.076;99.996
2021-05;chrome;37.944;47.056
2021-05;Discord;2.488;17.848
2021-05;csgo;0.500;18.004
2021-05;"Lunar Client";0.412;5.560
2021-05;explorer;0.176;3.856
2021-05;steam;0.012;2.428
2021-05;GTA5;0.004;1.316
2021-05;RussiaPhobia;0.000;0.708
2021-05;FaceIt;0.004;0.660
2021-05;Fortnite;0.056;0.416
2021-05;NordVPN;0.228;0.124
2021-05;EpicGamesLauncher;0.012;0.304

Requirements
------------
OS: Windows 7, 10
Visual C++ Redistributable for Visual Studio 2015 (MSVCP140.dll, VCRUNTIME140.dll)

Note:
The functionality cannot be implemented as (system) service as API function cannot provide information about
user applications (active window, etc.). So the workaround is hidden application under user account.

TODO:
Upload to gsheet
https://medium.com/craftsmenltd/from-csv-to-google-sheet-using-python-ef097cb014f9
Google Drive API
https://developers.google.com/drive/api/v3/reference/files/update
Google Sheet API
https://developers.google.com/sheets/api
OAuth2 C#
https://github.com/googlesamples/oauth-apps-for-windows/tree/master/OAuthConsoleApp