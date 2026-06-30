param ([string]$BasePath = ".\")

$Host.SetShouldExit(111) # Set non-zero return code until task successfully finished
$ErrorActionPreference = "Stop" # Stop on any error

Remove-Variable MHD_ver,MHD_ver_major,MHD_ver_minor,MHD_ver_patchlev -ErrorAction:SilentlyContinue

Write-Output "Processing: ${BasePath}..\..\configure.ac"
foreach($line in Get-Content "${BasePath}..\..\configure.ac")
{
    if ($line -match '^AC_INIT\(\[(?:GNU )?libmicrohttpd2\], *\[((\d+).(\d+).(\d+)) *\]') 
    {
        [string]$MHD_ver = $Matches[1].ToString()
        [string]$MHD_ver_major = $Matches[2].ToString()
        [string]$MHD_ver_minor = $Matches[3].ToString()
        [string]$MHD_ver_patchlev = $Matches[4].ToString()
        break 
    }
}
if ("$MHD_ver" -eq "" -or "$MHD_ver_major" -eq ""  -or "$MHD_ver_minor" -eq "" -or "$MHD_ver_patchlev" -eq "")
{
    Write-Error -Message ("error MHDVSVER01 : Can't find MHD version")
    Throw ($MyInvocation.MyCommand.Name + " : error MHDVSVER01 : Can't find MHD version")
}

Write-Output "Detected MHD version: $MHD_ver"

Write-Output "Generating ${BasePath}microhttpd2.rc"
Get-Content "${BasePath}microhttpd2.rc.in" | ForEach-Object {
    $_  -replace '@PACKAGE_VERSION_MAJOR@',"$MHD_ver_major" `
        -replace '@PACKAGE_VERSION_MINOR@', "$MHD_ver_minor" `
        -replace '@PACKAGE_VERSION_SUBMINOR@', "$MHD_ver_patchlev" `
        -replace '@PACKAGE_VERSION@', "$MHD_ver"
} | Out-File -FilePath "${BasePath}microhttpd2.rc" -Force

$Host.SetShouldExit(0) # Reset return code

Write-Output "${BasePath}microhttpd_dll_res_vc.rc was generated "
exit 0 # Exit with success code
