param([string]$DenoVersion = 'v2.9.8', [string]$DownloadCache = '')
$ErrorActionPreference = 'Stop'
$ProgressPreference = 'SilentlyContinue'
$projectRoot = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
$toolsDirectory = Join-Path $projectRoot 'Header_Files\ytdl'
$setupDirectory = Join-Path $projectRoot ('build\tools-setup-' + [guid]::NewGuid().ToString('N'))
if ($DownloadCache) {
    $setupDirectory = [IO.Path]::GetFullPath((Join-Path $projectRoot $DownloadCache))
    $buildPrefix = [IO.Path]::GetFullPath((Join-Path $projectRoot 'build')) + '\'
    if (-not $setupDirectory.StartsWith($buildPrefix, [StringComparison]::OrdinalIgnoreCase)) { throw 'Download cache must stay inside the project build folder.' }
}
New-Item -ItemType Directory -Force -Path $setupDirectory,$toolsDirectory | Out-Null

# All downloads stay in this project; this does not install software globally.
$ytDownload = Join-Path $setupDirectory 'yt-dlp.exe'
$ytChecksums = Join-Path $setupDirectory 'SHA2-256SUMS'
if (-not (Test-Path -LiteralPath $ytDownload)) { Invoke-WebRequest -UseBasicParsing -Uri 'https://github.com/yt-dlp/yt-dlp/releases/latest/download/yt-dlp.exe' -OutFile $ytDownload }
if (-not (Test-Path -LiteralPath $ytChecksums)) { Invoke-WebRequest -UseBasicParsing -Uri 'https://github.com/yt-dlp/yt-dlp/releases/latest/download/SHA2-256SUMS' -OutFile $ytChecksums }
$checksumLine = Get-Content -LiteralPath $ytChecksums | Where-Object { $_ -match '\s+\*?yt-dlp\.exe$' } | Select-Object -First 1
if (-not $checksumLine) { throw 'Missing yt-dlp checksum.' }
$expectedHash = ($checksumLine -split '\s+')[0]
if ((Get-FileHash -LiteralPath $ytDownload -Algorithm SHA256).Hash -ne $expectedHash) { throw 'yt-dlp checksum mismatch.' }

$archive = Join-Path $setupDirectory 'deno.zip'
$checksum = Join-Path $setupDirectory 'deno.sha256sum'
$releaseBase = 'https://github.com/denoland/deno/releases/download/' + $DenoVersion
if (-not (Test-Path -LiteralPath $archive)) { Invoke-WebRequest -UseBasicParsing -Uri ($releaseBase + '/deno-x86_64-pc-windows-msvc.zip') -OutFile $archive }
if (-not (Test-Path -LiteralPath $checksum)) { Invoke-WebRequest -UseBasicParsing -Uri ($releaseBase + '/deno-x86_64-pc-windows-msvc.zip.sha256sum') -OutFile $checksum }
# Deno's Windows release uses PowerShell's Hash: format, unlike Unix sums.
$expectedHash = [regex]::Match((Get-Content -LiteralPath $checksum -Raw), '(?i)\b[0-9a-f]{64}\b').Value
if (-not $expectedHash) { throw 'Missing Deno checksum.' }
if ((Get-FileHash -LiteralPath $archive -Algorithm SHA256).Hash -ne $expectedHash) { throw 'Deno checksum mismatch.' }
Expand-Archive -LiteralPath $archive -DestinationPath (Join-Path $setupDirectory 'deno') -Force

$previousDownloader = Join-Path $toolsDirectory 'yt-dlp.exe'
if (Test-Path -LiteralPath $previousDownloader) {
    Copy-Item -LiteralPath $previousDownloader -Destination (Join-Path $setupDirectory 'yt-dlp.previous.exe')
}
Copy-Item -LiteralPath $ytDownload -Destination $previousDownloader -Force
Copy-Item -LiteralPath (Join-Path $setupDirectory 'deno\deno.exe') -Destination (Join-Path $toolsDirectory 'deno.exe') -Force
& $previousDownloader --version
& (Join-Path $toolsDirectory 'deno.exe') --version
Write-Output 'Downloader updated; local Deno runtime installed. Existing FFmpeg files retained.'
