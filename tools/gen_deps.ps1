param(
    [string]$TmpFile,
    [string]$ObjFile,
    [string]$SrcFile,
    [string]$DepFile
)

$lines = Get-Content -LiteralPath $TmpFile -Encoding Default

foreach ($line in $lines) {
    # Ignore toute ligne /showIncludes "... : C:\...\fichier" (peu importe le préfixe/langue/extension,
    # les headers standards comme <optional> n'ont pas d'extension)
    if ($line -notmatch ':\s*[A-Za-z]:\\') {
        Write-Host $line
    }
}

$deps = $lines | Where-Object { $_ -match ':\s*([A-Za-z]:\\.*\.(hpp|h))\s*$' } | ForEach-Object {
    if ($_ -match ':\s*([A-Za-z]:\\.*\.(hpp|h))\s*$') {
        $matches[1].Trim()
    }
} | Where-Object { $_ -and ($_ -match '\\src\\') } | Sort-Object -Unique

$depsStr = ($deps -join ' ')
"$ObjFile`: $SrcFile $depsStr" | Set-Content -LiteralPath $DepFile

Remove-Item -LiteralPath $TmpFile -Force