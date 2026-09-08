# SVL-Connect Packaging Script for Windows x64
$ErrorActionPreference = "Stop"

$workspace = "C:\svl-network\svl-connect"
$buildDir = "$workspace\build"
$distBase = "$workspace\dist"
$version = "v1.0.4"
$distFolder = "$distBase\SVL-Connect-$version-Windows-x64"
$zipPath = "$distBase\SVL-Connect-$version-Windows-x64-Portable.zip"

Write-Host "Creating output directory: $distFolder..."
if (Test-Path $distFolder) {
    Remove-Item -Recurse -Force $distFolder
}
if (Test-Path $zipPath) {
    Remove-Item -Force $zipPath
}
New-Item -ItemType Directory -Force -Path $distFolder | Out-Null

Write-Host "Copying core executables and assets..."
Copy-Item "$buildDir\svl-connect.exe" "$distFolder\svl-connect.exe" -Force
if (Test-Path "$buildDir\svl-connect_filelink.exe") {
    Copy-Item "$buildDir\svl-connect_filelink.exe" "$distFolder\svl-connect_filelink.exe" -Force
}
if (Test-Path "$buildDir\jars") {
    Copy-Item -Recurse "$buildDir\jars" "$distFolder\jars" -Force
}
if (Test-Path "$workspace\launcher\qtlogging.ini") {
    Copy-Item "$workspace\launcher\qtlogging.ini" "$distFolder\qtlogging.ini" -Force
}
if (Test-Path "$workspace\program_info\portable.txt") {
    Copy-Item "$workspace\program_info\portable.txt" "$distFolder\portable.txt" -Force
}

Write-Host "Running windeployqt..."
$windeployqt = "C:\Qt\6.6.3\msvc2019_64\bin\windeployqt.exe"
& $windeployqt --release --no-translations --compiler-runtime --svg "$distFolder\svl-connect.exe"

Write-Host "Copying MSVC Runtime & OpenMP DLLs..."
$crtDir = "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Redist\MSVC\14.51.36231\x64\Microsoft.VC145.CRT"
if (Test-Path $crtDir) {
    Get-ChildItem -Path $crtDir -Filter "*.dll" | ForEach-Object {
        Copy-Item -Path $_.FullName -Destination "$distFolder\" -Force
    }
}
$openmpDir = "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Redist\MSVC\14.51.36231\x64\Microsoft.VC145.OpenMP"
if (Test-Path $openmpDir) {
    Get-ChildItem -Path $openmpDir -Filter "*.dll" | ForEach-Object {
        Copy-Item -Path $_.FullName -Destination "$distFolder\" -Force
    }
}

Write-Host "Writing qt.conf..."
$qtConfContent = "[Paths]`r`nPrefix = .`r`nPlugins = .`r`n"
[System.IO.File]::WriteAllText("$distFolder\qt.conf", $qtConfContent)

Write-Host "Packaging into ZIP archive: $zipPath..."
Compress-Archive -Path "$distFolder\*" -DestinationPath "$zipPath" -CompressionLevel Optimal

$zipItem = Get-Item $zipPath
Write-Host "Package completed successfully!"
Write-Host "Output ZIP: $($zipItem.FullName) ($([math]::Round($zipItem.Length / 1MB, 2)) MB)"

$downloadsDir = "C:\svl-network\svl-master-api\public\downloads"
if (Test-Path $downloadsDir) {
    Copy-Item $zipPath "$downloadsDir\SVL-Connect-$version-Windows-x64-Portable.zip" -Force
    Copy-Item $zipPath "$downloadsDir\svl-connect-windows-x64.zip" -Force
    Write-Host "Copied release ZIP to master API downloads portal!"
}
