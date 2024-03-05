@echo off

set EngineVersion=5.1

for /f "skip=2 tokens=2*" %%a in ('reg query "HKEY_LOCAL_MACHINE\SOFTWARE\EpicGames\Unreal Engine\%EngineVersion%" /v "InstalledDirectory"') do set "EngineDirectory=%%b"

set AutomationToolPath="%EngineDirectory%\Engine\Build\BatchFiles\RunUAT.bat"
set PluginPath="%cd%\JSBSimFlightDynamicsModel.uplugin"
set OutputPath="%cd%\..\..\..\UE\%EngineVersion%\JSBSimFlightDynamicsModel"

title Build Plugin
echo Automation Tool Path: %AutomationToolPath%
echo:

call %AutomationToolPath% BuildPlugin -Plugin=%PluginPath% -Package=%OutputPath% -Rocket -TargetPlatforms=Win64
echo:
pause
exit 0
