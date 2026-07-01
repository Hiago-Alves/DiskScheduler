<#
.SYNOPSIS
    Script de build do DiskScheduler para PowerShell (Windows), equivalente
    ao Makefile, para quando o utilitario "make" nao esta instalado.

.DESCRIPTION
    Compila cada arquivo .c de src/ em um .o dentro de build/, e depois
    linka todos os objetos no executavel disk_scheduler.exe na raiz do
    projeto. Suporta os mesmos "alvos" do Makefile como parametro:

        .\build.ps1            -> build incremental (padrao)
        .\build.ps1 run        -> compila (se necessario) e executa
        .\build.ps1 clean      -> remove build/ e o executavel
        .\build.ps1 rebuild    -> clean + build completo
        .\build.ps1 -DebugBuild -> build com simbolos de debug (-g -O0)

.NOTES
    Padrao: C11 (gcc -std=c11)
#>

param(
    [Parameter(Position = 0)]
    [ValidateSet("build", "run", "clean", "rebuild")]
    [string]$Target = "build",

    # Nome "DebugBuild" (e nao "Debug") de proposito: "-Debug" e um
    # parametro COMUM reservado pelo PowerShell (junto com -Verbose,
    # -ErrorAction etc.) e e adicionado automaticamente a qualquer
    # script/funcao com bloco param() -- usar esse nome geraria conflito
    # ("parametro definido varias vezes"), que foi exatamente o erro
    # visto ao rodar a versao anterior deste script.
    [switch]$DebugBuild
)

$ErrorActionPreference = "Stop"

# ---------------------------------------------------------------------------
# Configuracao (equivalente as variaveis do Makefile)
# ---------------------------------------------------------------------------

$CC        = "gcc"
$SrcDir    = "src"
$BuildDir  = "build"
$TargetExe = "disk_scheduler.exe"

# -std=c11 -Wall -Wextra -Iinclude: mesmas flags do Makefile.
# Nao usamos -MMD/-MP aqui (isso e recurso do make); em compensacao,
# este script sempre recompila arquivos cujo .c ou cujo .h dependido
# seja mais novo que o .o -- ver logica de "precisa recompilar?" abaixo.
$CFlags = @("-std=c11", "-Wall", "-Wextra", "-Iinclude")

if ($DebugBuild) {
    $CFlags += @("-g", "-O0", "-DDEBUG")
} else {
    $CFlags += @("-O2")
}

# ---------------------------------------------------------------------------
# Funcao: limpar artefatos de build
# ---------------------------------------------------------------------------

function Invoke-Clean {
    Write-Host "Limpando artefatos de build..." -ForegroundColor Cyan
    if (Test-Path $BuildDir)  { Remove-Item -Recurse -Force $BuildDir }
    if (Test-Path $TargetExe) { Remove-Item -Force $TargetExe }
}

# ---------------------------------------------------------------------------
# Funcao: build (compilar .c -> .o e linkar)
# ---------------------------------------------------------------------------

function Invoke-Build {
    if (-not (Test-Path $SrcDir)) {
        Write-Error "Diretorio '$SrcDir' nao encontrado. Rode este script a partir da raiz do projeto."
    }

    if (-not (Test-Path $BuildDir)) {
        New-Item -ItemType Directory -Path $BuildDir | Out-Null
    }

    # Pega todos os .c diretamente dentro de src/ (nao entra em subpastas
    # como src/tests ou src/output, igual ao Makefile).
    $sources = Get-ChildItem -Path $SrcDir -Filter "*.c" -File

    if ($sources.Count -eq 0) {
        Write-Error "Nenhum arquivo .c encontrado em '$SrcDir'."
    }

    # Todos os headers de include/, usados para decidir se um .o precisa
    # ser recompilado mesmo que o .c correspondente nao tenha mudado
    # (aproximacao simples do que o -MMD/-MP do make faz automaticamente).
    $headers = @(Get-ChildItem -Path "include" -Filter "*.h" -File -ErrorAction SilentlyContinue)
    $newestHeaderTime = if ($headers.Count -gt 0) {
        ($headers | Measure-Object -Property LastWriteTime -Maximum).Maximum
    } else {
        [DateTime]::MinValue
    }

    $objects = @()
    $anyCompiled = $false

    foreach ($src in $sources) {
        $objPath = Join-Path $BuildDir ($src.BaseName + ".o")
        $objects += $objPath

        $needsRebuild = $true
        if (Test-Path $objPath) {
            $objTime = (Get-Item $objPath).LastWriteTime
            # So reaproveita o .o existente se ele for mais novo que o
            # .c E mais novo que o header mais recentemente alterado.
            if ($objTime -gt $src.LastWriteTime -and $objTime -gt $newestHeaderTime) {
                $needsRebuild = $false
            }
        }

        if ($needsRebuild) {
            Write-Host "Compilando $($src.FullName)..." -ForegroundColor Yellow
            & $CC @CFlags -c $src.FullName -o $objPath
            if ($LASTEXITCODE -ne 0) {
                Write-Error "Falha ao compilar $($src.Name)."
            }
            $anyCompiled = $true
        }
    }

    $exeExists = Test-Path $TargetExe
    if ($anyCompiled -or -not $exeExists) {
        Write-Host "Linkando $TargetExe..." -ForegroundColor Cyan
        & $CC @objects -o $TargetExe
        if ($LASTEXITCODE -ne 0) {
            Write-Error "Falha ao linkar $TargetExe."
        }
        Write-Host "Build concluido: $TargetExe" -ForegroundColor Green
    } else {
        Write-Host "Nada a fazer: $TargetExe ja esta atualizado." -ForegroundColor Green
    }
}

# ---------------------------------------------------------------------------
# Dispatcher de alvos (equivalente aos targets do Makefile)
# ---------------------------------------------------------------------------

switch ($Target) {
    "clean" {
        Invoke-Clean
    }
    "rebuild" {
        Invoke-Clean
        Invoke-Build
    }
    "run" {
        Invoke-Build
        Write-Host ""
        & ".\$TargetExe"
    }
    default {
        Invoke-Build
    }
}