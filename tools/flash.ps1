<#
  Graba el paquete verificado y conserva antes una imagen completa de 16 MB.
  El parámetro Port se limita a COM<número> para evitar destinos accidentales.
#>
param([Parameter(Mandatory=$true)][ValidatePattern('^COM[0-9]+$')][string]$Port)
$ErrorActionPreference='Stop'
Write-Host 'RADAR V3.12: paquete exclusivo para Freenove FNK0104N 3.5 pulgadas.'
$projectRoot=Split-Path $PSScriptRoot -Parent
$pioPython=Join-Path $env:USERPROFILE '.platformio\penv\Scripts\python.exe'
$esptool=Join-Path $env:USERPROFILE '.platformio\packages\tool-esptoolpy\esptool.py'
if(!(Test-Path -LiteralPath $pioPython) -or !(Test-Path -LiteralPath $esptool)){throw 'No se encuentra PlatformIO/esptool local. Compila una vez con PlatformIO o instala sus herramientas.'}
$bins=Join-Path $projectRoot 'bin'
foreach($name in @('bootloader.bin','partitions.bin','boot_app0.bin','firmware.bin','littlefs.bin')){
 if(!(Test-Path -LiteralPath (Join-Path $bins $name))){throw "Falta bin/$name"}
}
# La integridad se revisa antes de tocar el hardware.
$expected=Get-Content -LiteralPath (Join-Path $bins 'sha256.json') -Raw | ConvertFrom-Json
foreach($entry in $expected.PSObject.Properties){
 $actual=(Get-FileHash -LiteralPath (Join-Path $bins $entry.Name) -Algorithm SHA256).Hash.ToLowerInvariant()
 if($actual -ne $entry.Value){throw "Integridad incorrecta: $($entry.Name). No se ha accedido a la placa."}
}
# La copia contiene bootloader, programa, LittleFS y NVS de la placa destino.
$backupDir=Join-Path $projectRoot 'backups'
New-Item -ItemType Directory -Force -Path $backupDir | Out-Null
$backupFile=Join-Path $backupDir ('fnk0104n-backup-'+(Get-Date -Format 'yyyyMMdd-HHmmss')+'.bin')
Write-Host "Copiando los 16 MB originales a $backupFile"
& $pioPython $esptool --chip esp32s3 --port $Port --baud 460800 read_flash 0x0 0x1000000 $backupFile
if($LASTEXITCODE -ne 0){throw 'Copia fallida; no se ha iniciado la grabacion.'}
if((Get-Item -LiteralPath $backupFile).Length -ne 16777216){throw 'Tamano de copia incorrecto; se cancela la grabacion.'}
Get-FileHash -LiteralPath $backupFile -Algorithm SHA256 | Format-List
# Preserve the generated bootloader header (DIO). The bootloader enables QIO
# afterwards; forcing QIO here prevents the ESP32-S3 ROM from loading it.
& $pioPython $esptool --chip esp32s3 --port $Port --baud 460800 write_flash --flash_mode keep --flash_freq keep --flash_size keep 0x0 (Join-Path $bins 'bootloader.bin') 0x8000 (Join-Path $bins 'partitions.bin') 0xe000 (Join-Path $bins 'boot_app0.bin') 0x10000 (Join-Path $bins 'firmware.bin') 0x610000 (Join-Path $bins 'littlefs.bin')
if($LASTEXITCODE -ne 0){throw 'Error de grabacion. Conserva la copia y vuelve a entrar en modo BOOT.'}
Write-Host 'Grabacion terminada. Pulsa RESET si es necesario. Consulta docs/CABLEADO.md.'
