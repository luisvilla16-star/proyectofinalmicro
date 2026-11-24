# Terminal serie para jugar PATTERNIX con el teclado de la PC.
# Manda cada tecla EN CUANTO SE OPRIME, sin Enter, y muestra lo que contesta la tarjeta.
#
#   Lo mas facil: doble clic en jugar.bat
#   A mano:       powershell -ExecutionPolicy Bypass -File terminal.ps1          (busca el puerto)
#                 powershell -ExecutionPolicy Bypass -File terminal.ps1 COM5     (puerto fijo)
#                 powershell -ExecutionPolicy Bypass -File terminal.ps1 -Eco     (sin tarjeta: solo
#                                                           muestra la tecla que mandaria)
#
# Es el mismo puerto serie que usa cualquier terminal: 9600 baudios, 8 bits de datos,
# sin paridad y 1 bit de parada. Salir: Esc.
param([string]$Puerto = '', [switch]$Prueba, [switch]$Eco)
$Host.UI.RawUI.WindowTitle = "PATTERNIX"
if ($Eco) {
    Write-Host 'Modo eco (sin tarjeta): oprime teclas, sin Enter. Esc sale.' -ForegroundColor Green
    while ($true) {
        if ([Console]::KeyAvailable) {
            $tecla = [Console]::ReadKey($true)
            if ($tecla.Key -eq [ConsoleKey]::Escape) { break }
            if ($tecla.KeyChar -ne [char]0) { [Console]::WriteLine("se mandaria: $($tecla.KeyChar)") }
        }
        Start-Sleep -Milliseconds 5
    }
    exit 0
}

if (-not $Puerto) {
    $serie = @(Get-CimInstance Win32_PnPEntity | Where-Object { $_.Name -match '\(COM\d+\)' })
    $tarjeta = $serie | Where-Object { $_.Name -match 'CH340|Arduino|USB-SERIAL|USB Serial' } | Select-Object -First 1
    if (-not $tarjeta) { $tarjeta = $serie | Select-Object -First 1 }
    if (-not $tarjeta) {
        Write-Host 'No encontre ningun puerto COM. Conecta la tarjeta por USB y vuelve a intentar.' -ForegroundColor Red
        exit 1
    }
    $null = $tarjeta.Name -match '\((COM\d+)\)'
    $Puerto = $Matches[1]
    Write-Host "Tarjeta encontrada: $($tarjeta.Name)" -ForegroundColor DarkGray
}

$sp = New-Object System.IO.Ports.SerialPort $Puerto, 9600, 'None', 8, 'One'
$sp.DtrEnable = $true                    # como cualquier terminal: al abrir, la UNO se reinicia

# El puerto solo lo puede tener abierto un programa. Si esta ocupado, se avisa
# y se vuelve a intentar cuando la persona lo libere.
while (-not $sp.IsOpen) {
    try { $sp.Open() }
    catch {
        Write-Host ''
        Write-Host "No pude abrir ${Puerto}: otro programa lo tiene abierto." -ForegroundColor Red
        Write-Host '  - Data Visualizer de Microchip Studio: boton Disconnect (o cierra esa pestana).' -ForegroundColor Yellow
        Write-Host '  - Monitor serie de Arduino: cierralo.' -ForegroundColor Yellow
        Write-Host '  - Si se esta cargando el programa, espera a que termine.' -ForegroundColor Yellow
        if ($Prueba) { exit 1 }
        Write-Host 'Liberalo y oprime una tecla para reintentar (Esc para salir)...' -ForegroundColor Yellow
        if ([Console]::ReadKey($true).Key -eq [ConsoleKey]::Escape) { exit 1 }
    }
}

try {
    if ($Prueba) {
        # Sin teclado: espera el saludo, pide el ranking y termina
        Start-Sleep -Milliseconds 3200
        [Console]::Write($sp.ReadExisting())
        $sp.Write('R'); Start-Sleep -Milliseconds 500
        [Console]::Write($sp.ReadExisting())
        return
    }

    Write-Host "Conectado a ${Puerto}: 9600 baudios, 8 bits, sin paridad, 1 bit de parada." -ForegroundColor Green
    while ($true) {
        if ($sp.BytesToRead -gt 0) { [Console]::Write($sp.ReadExisting()) }
        if ([Console]::KeyAvailable) {
            $tecla = [Console]::ReadKey($true)
            if ($tecla.Key -eq [ConsoleKey]::Escape) { break }
            if ($tecla.KeyChar -ne [char]0) { $sp.Write([string]$tecla.KeyChar) }
        }
        else { Start-Sleep -Milliseconds 2 }
    }
}
finally {
    if ($sp.IsOpen) { $sp.Close() }
}
