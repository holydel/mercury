# PowerShell script to fix JNI libs directory structure for Android build
param(
    [Parameter(Mandatory=$true)]
    [string]$SourceDir
)

Write-Host "Fixing JNI libs structure in: $SourceDir"

if (-not (Test-Path $SourceDir)) {
    Write-Host "Source directory does not exist: $SourceDir"
    exit 0
}

# Get absolute path for proper comparisons
$SourceDirAbsolute = Resolve-Path -Path $SourceDir

# Find all .so files
$soFiles = Get-ChildItem -Path $SourceDirAbsolute -Filter "*.so" -Recurse

foreach ($file in $soFiles) {
    $relativePath = $file.FullName.Substring($SourceDirAbsolute.Path.Length + 1)
    $pathParts = $relativePath -split "[\\/]"
    
    # Find valid ABI name in the path
    $validAbis = @('arm64-v8a', 'armeabi-v7a', 'x86', 'x86_64')
    $abiName = $null
    
    foreach ($part in $pathParts) {
        if ($validAbis -contains $part) {
            $abiName = $part
            break
        }
    }
    
    if ($abiName) {
        $targetDir = Join-Path $SourceDirAbsolute $abiName
        $targetFile = Join-Path $targetDir $file.Name
        
        # Create target directory if it doesn't exist
        if (-not (Test-Path $targetDir)) {
            New-Item -ItemType Directory -Path $targetDir -Force | Out-Null
        }
        
        # Copy file if it's not already in the right place
        # Check if the source file is already in the correct ABI directory at the root level
        $isAlreadyInCorrectLocation = ($file.Directory.FullName -eq $targetDir)
        
        if (-not $isAlreadyInCorrectLocation) {
            Write-Host "Moving $($file.FullName) to $targetFile"
            Copy-Item -Path $file.FullName -Destination $targetFile -Force
        }
    }
}

# Remove any directories that have invalid ABI names (but contain valid ones)
$invalidDirs = Get-ChildItem -Path $SourceDirAbsolute -Directory | Where-Object { 
    $_.Name -notmatch '^(arm64-v8a|armeabi-v7a|x86|x86_64)$' 
}

foreach ($dir in $invalidDirs) {
    Write-Host "Removing invalid ABI directory: $($dir.FullName)"
    Remove-Item -Path $dir.FullName -Recurse -Force
}

Write-Host "JNI libs structure fixed successfully"