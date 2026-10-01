#Requires -Version 5.1
<#
.SYNOPSIS
Downloads and verifies the proving parameters used by KmdClassic.
.DESCRIPTION
Uses only built-in PowerShell commands. Existing files are verified before
being reused. Downloads are verified before replacing the destination file.
.PARAMETER ParamsDir
Destination directory. Defaults to %APPDATA%\ZcashParams on Windows.
.EXAMPLE
powershell.exe -NoProfile -ExecutionPolicy Bypass -File .\zcutil\fetch-params.ps1
#>
[CmdletBinding()]
param(
    [string]$ParamsDir = (Join-Path ([Environment]::GetFolderPath('ApplicationData')) 'ZcashParams'),
    [ValidateRange(1, 10)]
    [int]$Attempts = 3
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
# Rendering per-buffer progress in Windows PowerShell 5.1 slows large downloads.
$ProgressPreference = 'SilentlyContinue'
$baseUrl = 'https://download.z.cash/downloads'
$parameters = @(
    @{ Name = 'sapling-spend.params'; Hash = '8e48ffd23abb3a5fd9c5589204f32d9c31285a04b78096ba40a79b75677efc13' },
    @{ Name = 'sapling-output.params'; Hash = '2f0ebbcbb9bb0bcffe95a397e7eba89c29eb4dde6191c339db88570e3f3fb0e4' },
    @{ Name = 'sprout-groth16.params'; Hash = 'b685d700c60328498fbde589c8c7c484c722b788b265b72af448a5bf0ee55b50' }
)

$lock = $null
$download = $null
$originalProtocol = [Net.ServicePointManager]::SecurityProtocol
try {
    [Net.ServicePointManager]::SecurityProtocol = $originalProtocol -bor [Net.SecurityProtocolType]::Tls12
    $ParamsDir = $ExecutionContext.SessionState.Path.GetUnresolvedProviderPathFromPSPath($ParamsDir)
    [IO.Directory]::CreateDirectory($ParamsDir) | Out-Null
    try {
        $lock = [IO.File]::Open((Join-Path $ParamsDir 'fetch-params.lock'),
            [IO.FileMode]::OpenOrCreate, [IO.FileAccess]::ReadWrite, [IO.FileShare]::None)
    } catch {
        throw "Cannot lock the parameter directory. Another download may be running, or the directory is not writable: $ParamsDir"
    }

    Write-Host "Parameter directory: $ParamsDir"
    foreach ($parameter in $parameters) {
        $destination = Join-Path $ParamsDir $parameter.Name
        if (Test-Path -LiteralPath $destination -PathType Leaf) {
            Write-Host "Verifying $($parameter.Name)..."
            if ((Get-FileHash -LiteralPath $destination -Algorithm SHA256).Hash -eq $parameter.Hash) {
                Write-Host 'Already present and valid.'
                continue
            }
            Write-Warning "Checksum mismatch: $($parameter.Name). Downloading a replacement."
        }

        $download = "$destination.download"
        for ($attempt = 1; $attempt -le $Attempts; $attempt++) {
            try {
                Write-Host "Downloading $($parameter.Name) (attempt $attempt/$Attempts)..."
                Invoke-WebRequest -UseBasicParsing -Uri "$baseUrl/$($parameter.Name)" -OutFile $download
                if ((Get-FileHash -LiteralPath $download -Algorithm SHA256).Hash -ne $parameter.Hash) {
                    throw "SHA-256 verification failed for $($parameter.Name)."
                }
                Move-Item -LiteralPath $download -Destination $destination -Force
                $download = $null
                Write-Host 'Downloaded and verified.'
                break
            } catch {
                if ($attempt -eq $Attempts) { throw }
                Write-Warning "Download failed: $($_.Exception.Message) Retrying in 3 seconds."
                Start-Sleep -Seconds 3
            }
        }
    }
    Write-Host 'All KmdClassic proving parameters are ready.'
} finally {
    if ($download -and (Test-Path -LiteralPath $download)) {
        Remove-Item -LiteralPath $download -Force -ErrorAction SilentlyContinue
    }
    # Keep the lock file; closing the handle releases the lock, including after a crash.
    if ($null -ne $lock) { $lock.Dispose() }
    [Net.ServicePointManager]::SecurityProtocol = $originalProtocol
}
