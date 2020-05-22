 **RunSync** utility
=========================

Deploy GUI app on server, install/run in client computers (Windows) with further version synchronization. 

1. Easy applications installation on client PCs
2. Automated monitoring versions and upgrade applications.

It does not required on target PC any specific privileges except what is requred for regular application to run.
No services installation requires.  


## Prerequisites 


Application binaries which going to be controlled by *RunSync* are to be placed on file server in shared folder accessible with READ-ONLY rights for deployment target PC.

Supposed that each application placed in one folder - *Deployment Source Folder - DSF* . 

Inside *DSF* each application is in its own separate folder named same as EXE-file name.

In the root of each application folder to be placed file with name *version*. See details about this file below.  


Example:

```bat
\\my-server\Clients\MyApp\MyApp.exe
                          \version   
                          \Assemblies\...
                          \Modules\...
                          \....  

```

## Application Installation


1. Copy *RunSync.exe* to destination computer. For ex, to *C:\MyApps* folder. Optionally can be copied configuration file *RunSync.json** (see desription below)
2. Run it on destination PC. 
 
If no parameters provided it will shows popup where to be defined source and destination folders for installation.

Input source and destination pathes and hit *Save* button.
(see below notices about parameters storing)

In furter application starts input of  these parameters will not be requested.

3. After that appears popup for selecting application you going to install

List of available applications will be automatically populated from *Deployment Source Folder*.

4. After selection of application hit *OK* button.

Installation will starts automatically. It will be indicated by banner appears in top-right corner of desktop.
  
During installation:

- All files will be copied to destination PC (with consistensy check by SHA1 hash)
- Will be made ShortCut on desktop to start application by RunSync utility (requires for version control)

Installation can be repeated for all applications wants to install to given PC.

## Starting application with version synchronization

You sould not start application EXE-file directly. Instead of this use command:

*RunSync \<application name\>   

Example:

```bat
C:\MyApps\RunSync MyApp1
```  

This command configured for ShortCuts which are automatically created on DeskTop during installation.
If run *RunSync* without parameters it will request to choose application you want to start.

> NOTICE: It is does not metter how many times you start RunSync. It will be the only one instance of this process
> running in background and manage all started apps.  

## Checking Application Statuses

In any time you can press *Ctrl+Shift+R* key combination to open popup with *RunSync* parameters and information about currently managed apps.
It uses global Windows keys hook and will be opened regardless to application now is active.

You can modify and save *Source* and *Destination* folders.

## Version Controlling/Synchronization

If application started with *RunSync* then it will be continously checked for new version.
Version detection based on file with name *version* placed in folder where is application EXE-file.
Any content change of this file is considered as trigger to upgrade binaries in destination deployment PC(s).

To make automatic upgrade of all applications in all PCs where runs *RunSync*:

1. Update binaries on server
2. Change *version* file in application folder on server

All *RunSync* instances eventually checking *version* files on server and in case of changes detection will automatically start upgrading.
Upgrading will starts only if given application is not active.
When  user will switch to another application, stop or minmize main window then Upgrading will be activated automatically.

User can see that Upgrade is in progress by banner appears in top-right corner of desktop.
After finishing upgrade, if before app was runned it will be automatically restarted.
If application main window was minimized this state will be restored after restarting.

Current version status and activities will be shown as was explaned above in *Ctrl+Shift+R* popup. 

## Notice about application parameters
 
 There are parameters: 
  - Deployment source Folder
  - Deployment destination folder
  - (Optional) Destination shared folder user name
  - (Optional) Destination shared folder password
  
 These parameters can be passed (in order according its priority)
 1. By *RunSync* starting command line

Example:
 ```
RunSync MyApp --src=\\my-server\MyApps --dst=C:\MyApps 
```

 2. Loaded from Windows Registry (where it is stored during installation)

> *HKEY_CURRENT_USER\SOFTWARE\RunSync*

 3. Loaded from *RunSync.json* file placed in same folder where is *RunSync.exe**  
 
Example:
```json
{
  "AppsSrc":"\\\\my-server\\Level2Clients",
  "AppsDst":"D:\\Level2\\Clients"
}
```
When user press *Save* button in parameters popup dialog it first trying to save
parameters into Registry. If fails - trying to save in JSON file.

JSON file can be convenient on site if you install apps into same path on destination computers.
In such case once need to prepare file and then copy/paste *RunSync.exe*  and *RunSync.json* 
to destination computers. Will not be necessary to input pathes for installation.

> **Note about source folder password management**
> 
> For security reason, password can be stored only in Windows Registry or in JSON-config file.
> It can not be input manually because encryption is used. 
> Can input and store it only in *Crl+Shift+R* popup window. Password encryption is computer-dependent.
> If you save password into JSON file on one PC and then move it to another - password can not be used in there.
> Will need to input/save password again with new PC encryption key. 


 




