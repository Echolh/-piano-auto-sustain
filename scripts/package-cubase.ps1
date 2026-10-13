param([Parameter(Mandatory=$true)][string]$NativeBuild,[Parameter(Mandatory=$true)][string]$DeliveryRoot)
$ErrorActionPreference='Stop'
$repo=(Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$release=Join-Path $DeliveryRoot '01_发布包'
$name='Piano Auto Sustain 1.0 Cubase Windows x64'
$package=Join-Path $release $name
if(Test-Path -LiteralPath $package){throw 'Package already exists; inspect before replacing.'}
New-Item -ItemType Directory -Path $package -Force | Out-Null
Copy-Item -LiteralPath (Join-Path $NativeBuild 'Release\Piano Auto Sustain Cubase.dll') -Destination $package
Copy-Item -LiteralPath (Join-Path $PSScriptRoot 'Install-Cubase.ps1') -Destination $package
@'
@echo off
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0Install-Cubase.ps1"
'@ | Set-Content -LiteralPath (Join-Path $package '安装.cmd') -Encoding ASCII
Copy-Item -LiteralPath (Join-Path $repo 'docs\QUICKSTART-zh.md') -Destination (Join-Path $package '使用说明.md')
Copy-Item -LiteralPath (Join-Path $repo 'docs\QUICKSTART-en.md') -Destination (Join-Path $package 'Quick Start.md')
Copy-Item -LiteralPath (Join-Path $repo 'docs\VALIDATION-1.0.0.md') -Destination (Join-Path $package '验证范围.md')
Copy-Item -LiteralPath (Join-Path $repo 'licenses') -Destination (Join-Path $package 'licenses') -Recurse
$hashes=Get-ChildItem -LiteralPath $package -File -Recurse | ForEach-Object {"$((Get-FileHash -LiteralPath $_.FullName -Algorithm SHA256).Hash.ToLower())  $($_.FullName.Substring($package.Length+1))"}
Set-Content -LiteralPath (Join-Path $package 'SHA256SUMS.txt') -Value $hashes -Encoding UTF8
Compress-Archive -LiteralPath $package -DestinationPath (Join-Path $release "$name.zip") -CompressionLevel Optimal
# Include the actual working source and tests, not an older Git HEAD.
$sourceFiles=Get-ChildItem -LiteralPath $repo -Force | Where-Object {$_.Name -notin @('.git','.superpowers','.build','build-portable','dist','__pycache__')}
Compress-Archive -LiteralPath $sourceFiles.FullName -DestinationPath (Join-Path $release 'Piano Auto Sustain 1.0 Source.zip') -CompressionLevel Optimal
Write-Output $package
