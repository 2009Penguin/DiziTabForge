param(
    [Parameter(Mandatory = $true)][string]$SourcePath,
    [Parameter(Mandatory = $true)][string]$TargetPath
)

Add-Type -AssemblyName System.Drawing

$sizes = @(16, 24, 32, 48, 64, 128, 256)
$pngSizes = @(64, 128, 256)

$sourceImage = [System.Drawing.Image]::FromFile((Resolve-Path $SourcePath))
$entries = New-Object System.Collections.Generic.List[object]

foreach ($size in $sizes) {
    $bitmap = New-Object System.Drawing.Bitmap($size, $size, [System.Drawing.Imaging.PixelFormat]::Format32bppArgb)
    $graphics = [System.Drawing.Graphics]::FromImage($bitmap)
    $graphics.InterpolationMode = [System.Drawing.Drawing2D.InterpolationMode]::HighQualityBicubic
    $graphics.PixelOffsetMode = [System.Drawing.Drawing2D.PixelOffsetMode]::HighQuality
    $graphics.SmoothingMode = [System.Drawing.Drawing2D.SmoothingMode]::HighQuality
    $graphics.Clear([System.Drawing.Color]::Transparent)
    $graphics.DrawImage($sourceImage, (New-Object System.Drawing.Rectangle(0, 0, $size, $size)))
    $graphics.Dispose()

    if ($pngSizes -contains $size) {
        $stream = New-Object System.IO.MemoryStream
        $bitmap.Save($stream, [System.Drawing.Imaging.ImageFormat]::Png)
        $payload = $stream.ToArray()
        $stream.Dispose()
    } else {
        $rect = New-Object System.Drawing.Rectangle(0, 0, $size, $size)
        $data = $bitmap.LockBits($rect, [System.Drawing.Imaging.ImageLockMode]::ReadOnly,
            [System.Drawing.Imaging.PixelFormat]::Format32bppArgb)
        $pixels = New-Object byte[] ($data.Stride * $size)
        [System.Runtime.InteropServices.Marshal]::Copy($data.Scan0, $pixels, 0, $pixels.Length)
        $bitmap.UnlockBits($data)

        $maskStride = [int]([Math]::Floor(($size + 31) / 32) * 4)
        $stream = New-Object System.IO.MemoryStream
        $writer = New-Object System.IO.BinaryWriter($stream)
        $writer.Write([uint32]40)
        $writer.Write([int32]$size)
        $writer.Write([int32]($size * 2))
        $writer.Write([uint16]1)
        $writer.Write([uint16]32)
        $writer.Write([uint32]0)
        $writer.Write([uint32]($size * $size * 4))
        $writer.Write([int32]0)
        $writer.Write([int32]0)
        $writer.Write([uint32]0)
        $writer.Write([uint32]0)
        for ($y = $size - 1; $y -ge 0; $y--) {
            for ($x = 0; $x -lt $size; $x++) {
                $offset = $y * $data.Stride + $x * 4
                $writer.Write($pixels[$offset + 2])
                $writer.Write($pixels[$offset + 1])
                $writer.Write($pixels[$offset])
                $writer.Write($pixels[$offset + 3])
            }
        }
        $writer.Write((New-Object byte[] ($maskStride * $size)))
        $writer.Flush()
        $payload = $stream.ToArray()
        $writer.Dispose()
        $stream.Dispose()
    }

    $bitmap.Dispose()
    $entries.Add([pscustomobject]@{ Size = $size; Payload = $payload })
}

$sourceImage.Dispose()

$output = New-Object System.IO.MemoryStream
$writer = New-Object System.IO.BinaryWriter($output)
$writer.Write([uint16]0)
$writer.Write([uint16]1)
$writer.Write([uint16]$entries.Count)
$offset = 6 + 16 * $entries.Count
foreach ($entry in $entries) {
    $dimension = if ($entry.Size -ge 256) { 0 } else { $entry.Size }
    $writer.Write([byte]$dimension)
    $writer.Write([byte]$dimension)
    $writer.Write([byte]0)
    $writer.Write([byte]0)
    $writer.Write([uint16]1)
    $writer.Write([uint16]32)
    $writer.Write([uint32]$entry.Payload.Length)
    $writer.Write([uint32]$offset)
    $offset += $entry.Payload.Length
}
foreach ($entry in $entries) {
    $writer.Write($entry.Payload)
}
$writer.Flush()
[System.IO.File]::WriteAllBytes($TargetPath, $output.ToArray())
$writer.Dispose()
$output.Dispose()

Write-Host ("wrote {0} ({1} bytes), sizes: {2}" -f $TargetPath,
    (Get-Item $TargetPath).Length, (($entries | ForEach-Object { $_.Size }) -join ', '))
