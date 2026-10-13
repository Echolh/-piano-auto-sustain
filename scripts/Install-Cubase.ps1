param([switch]$WaitForHostExit,[switch]$Quiet,[switch]$CheckOnly)
$ErrorActionPreference='Stop'
$source=Join-Path $PSScriptRoot 'Piano Auto Sustain Cubase.dll'
$expectedVersion='1.0.0'
if(!(Test-Path -LiteralPath $source)){throw 'Missing Piano Auto Sustain Cubase.dll'}
if((Get-Item -LiteralPath $source).VersionInfo.ProductVersion -ne $expectedVersion){throw 'Unexpected plugin version'}
$expectedHash=(Get-FileHash -LiteralPath $source -Algorithm SHA256).Hash
if($CheckOnly){Write-Output "Package OK: $expectedVersion $expectedHash";exit 0}
$identity=[Security.Principal.WindowsIdentity]::GetCurrent()
$admin=([Security.Principal.WindowsPrincipal]::new($identity)).IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)
if(!$admin){
    $arguments='-NoProfile -ExecutionPolicy Bypass -File "'+$PSCommandPath+'"'
    if($WaitForHostExit){$arguments+=' -WaitForHostExit'}
    if($Quiet){$arguments+=' -Quiet'}
    Start-Process powershell.exe -ArgumentList $arguments -Verb RunAs -WindowStyle Hidden
    exit
}
$log=Join-Path $PSScriptRoot '安装状态.txt'
function Status([string]$s){Set-Content -LiteralPath $log -Value ((Get-Date -Format 'yyyy-MM-dd HH:mm:ss')+' '+$s) -Encoding UTF8}
function HostRunning { @(Get-Process -ErrorAction SilentlyContinue | Where-Object {$_.ProcessName -match '^(Cubase|Nuendo)\d*'}).Count -gt 0 }
try {
    if(HostRunning){
        if(!$WaitForHostExit){throw '请先保存工程并退出 Cubase / Nuendo，再运行安装。'}
        Status '等待 Cubase / Nuendo 退出，随后自动安装 1.0.0；不会强制关闭宿主。'
        $deadline=(Get-Date).AddHours(4)
        while(HostRunning){if((Get-Date) -gt $deadline){throw '等待超时，请退出宿主后重新安装。'};Start-Sleep -Seconds 3}
    }
    $folder=Join-Path $env:CommonProgramFiles 'Steinberg\Shared Components'
    $target=Join-Path $folder 'Piano Auto Sustain Cubase.dll'
    New-Item -ItemType Directory -Path $folder -Force | Out-Null
    # A single plugin is the default workflow. Stop only our old Recorder.
    Get-Process -Name 'Piano Auto Sustain Recorder' -ErrorAction SilentlyContinue | Stop-Process -Force
    $run='HKCU:\Software\Microsoft\Windows\CurrentVersion\Run'
    $startup=(Get-ItemProperty -Path $run -Name 'Piano Auto Sustain' -ErrorAction SilentlyContinue).'Piano Auto Sustain'
    if($startup -like '*Piano Auto Sustain Recorder*'){Remove-ItemProperty -Path $run -Name 'Piano Auto Sustain'}
    Copy-Item -LiteralPath $source -Destination $target -Force
    if((Get-FileHash -LiteralPath $target -Algorithm SHA256).Hash -ne $expectedHash){throw 'Installed DLL verification failed'}
    # Only exact obsolete plugin filenames; never touch other Steinberg modules.
    foreach($name in @('BassPedalMidi.dll','BassPedalMidiProbe.dll','Bass Pedal MIDI Beta.dll','Bass Pedal MIDI Probe.dll')){
        $old=Join-Path $folder $name
        if(Test-Path -LiteralPath $old){Remove-Item -LiteralPath $old -Force}
    }
    Status "安装完成：1.0.0；SHA256 $expectedHash。现在可启动 Cubase。无需 Recorder。"
    if(!$Quiet){Add-Type -AssemblyName PresentationFramework;[System.Windows.MessageBox]::Show('安装完成。启动 Cubase，在钢琴轨道的 MIDI 插入中选择 Piano Auto Sustain Cubase。无需 Recorder。','Piano Auto Sustain 1.0.0') | Out-Null}
} catch {
    Status ('安装未完成：'+$_.Exception.Message)
    if(!$Quiet){Add-Type -AssemblyName PresentationFramework;[System.Windows.MessageBox]::Show($_.Exception.Message,'安装未完成') | Out-Null}
    exit 1
}
