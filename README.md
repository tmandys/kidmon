# KidMon

Utilities intended as free parental control tools to collect data about running applications to get info what apps are used and how long.

## Projects

### KidMon
It is Win32 (Windows 7/10) app running silently on background under user account and logging statistics hourly to CSV file. See [ReadMe.exe in project directory](./kidmon/ReadMe.txt) for more info.

### KidMonDude
Aggregation utility to make RRD .CSV fiels and .HTML charts using Google Charts. See [ReadMe.exe in project directory](./kidmondude/ReadMe.txt) for more info.

### KidMonLib
Shared library files

### KidMonService
It was intended implementation as Windows service but it is undoable this way. So it is left here as service template.

### Test
Just a code to test various API functions.

## History

### v0.1
* initial KidMon release

### v0.2
* KidMonDude for RRD aggregation and charts