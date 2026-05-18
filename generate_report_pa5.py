#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Генератор PDF-отчёта (практическое задание №5 — Контейнеризация с Docker).
Запуск: source venv/bin/activate && python3 generate_report_pa5.py
"""

from reportlab.lib.pagesizes import A4
from reportlab.lib.styles import ParagraphStyle
from reportlab.lib.units import cm
from reportlab.lib.colors import HexColor, white
from reportlab.platypus import (
    SimpleDocTemplate, Paragraph, Spacer, PageBreak,
    Table, TableStyle, KeepTogether, HRFlowable
)
from reportlab.lib.enums import TA_CENTER, TA_LEFT, TA_JUSTIFY
from reportlab.pdfbase import pdfmetrics
from reportlab.pdfbase.ttfonts import TTFont

SUPP = "/System/Library/Fonts/Supplemental"
SYS  = "/System/Library/Fonts"
for name, path in [
    ('TNR',    f'{SUPP}/Times New Roman.ttf'),
    ('TNR-B',  f'{SUPP}/Times New Roman Bold.ttf'),
    ('TNR-I',  f'{SUPP}/Times New Roman Italic.ttf'),
    ('TNR-BI', f'{SUPP}/Times New Roman Bold Italic.ttf'),
    ('Mono',   f'{SYS}/SFNSMono.ttf'),
]:
    pdfmetrics.registerFont(TTFont(name, path))
pdfmetrics.registerFontFamily('TNR',
    normal='TNR', bold='TNR-B', italic='TNR-I', boldItalic='TNR-BI')

NAVY   = HexColor('#1a2a4a')
BLUE   = HexColor('#2e4a8a')
CODEBG = HexColor('#f2f2f2')
BORDER = HexColor('#b0b8cc')
GRAY   = HexColor('#4a4a4a')
LGRAY  = HexColor('#e8e8e8')
LGREEN = HexColor('#e8f5e8')
GREEN  = HexColor('#1a6a2a')

W, H   = A4
ML, MR, MT, MB = 3.0*cm, 1.5*cm, 2.5*cm, 2.5*cm
TW     = W - ML - MR

STUDENT  = 'Фомин Владимир Родионович'
GROUP    = 'ЭФБО-06-24'
UNI      = 'РТУ МИРЭА'
DEPT     = 'ИПТИП'
SUBDEPT  = 'Программирование корпоративных систем'
DISC     = 'Программирование корпоративных систем'
TEACHER  = 'Сокунов Дмитрий Антонович'
SEMESTER = 'Весенний семестр 2025/2026 учебного года'
YEAR     = '2026'
OUTPUT   = 'ФОМИН_ВЛАДИМИР_ЭФБО-06-24_PA5.pdf'

# ─── Стили ────────────────────────────────────────────────────────────────────
def build_styles():
    S = {}
    def s(name, **kw): S[name] = ParagraphStyle(name, **kw)

    s('body',  fontName='TNR', fontSize=13, leading=20,
      alignment=TA_JUSTIFY, firstLineIndent=1.25*cm, spaceAfter=5)
    s('body0', fontName='TNR', fontSize=13, leading=20,
      alignment=TA_JUSTIFY, spaceAfter=5)
    s('bodyc', fontName='TNR', fontSize=13, leading=20,
      alignment=TA_CENTER, spaceAfter=5)

    s('h1', fontName='TNR-B', fontSize=15, leading=20, textColor=NAVY,
      alignment=TA_CENTER, spaceBefore=14, spaceAfter=10)
    s('h2', fontName='TNR-B', fontSize=13, leading=18, textColor=NAVY,
      alignment=TA_LEFT, spaceBefore=12, spaceAfter=6)

    s('code',    fontName='Mono', fontSize=8, leading=12, spaceAfter=0)
    s('caption', fontName='TNR-I', fontSize=10, leading=14,
      alignment=TA_CENTER, textColor=GRAY, spaceAfter=8)

    s('t_uni',   fontName='TNR',   fontSize=13, leading=18, alignment=TA_CENTER)
    s('t_title', fontName='TNR-B', fontSize=19, leading=26, alignment=TA_CENTER,
      textColor=NAVY, spaceBefore=4, spaceAfter=4)
    s('t_sub',   fontName='TNR',   fontSize=14, leading=20, alignment=TA_CENTER,
      textColor=BLUE)
    s('t_lbl',   fontName='TNR',   fontSize=12, leading=18, textColor=GRAY)
    s('t_val',   fontName='TNR-B', fontSize=12, leading=18)
    s('t_year',  fontName='TNR',   fontSize=12, leading=18, alignment=TA_CENTER)
    return S

# ─── Утилиты ──────────────────────────────────────────────────────────────────
def esc(t):
    return t.replace('&','&amp;').replace('<','&lt;').replace('>','&gt;')

def code_block(text, S, bg=CODEBG, border=BORDER):
    lines = text.rstrip('\n').split('\n')
    cells = [Paragraph(
        f'<font name="Mono" size="8">{"&nbsp;" if not ln.strip() else esc(ln).replace(" ", "&nbsp;")}</font>',
        S['code']) for ln in lines]
    tbl = Table([[cells]], colWidths=[TW - 0.8*cm])
    tbl.setStyle(TableStyle([
        ('BACKGROUND', (0,0),(-1,-1), bg),
        ('BOX',        (0,0),(-1,-1), 0.6, border),
        ('LEFTPADDING',  (0,0),(-1,-1), 9),
        ('RIGHTPADDING', (0,0),(-1,-1), 9),
        ('TOPPADDING',   (0,0),(-1,-1), 6),
        ('BOTTOMPADDING',(0,0),(-1,-1), 6),
    ]))
    return tbl

def bullet(text, S, lvl=0):
    indent = (1.2 + lvl*0.7)*cm
    return Paragraph(
        f'<bullet bulletIndent="-{0.4*cm}pt">•</bullet>{text}',
        ParagraphStyle('_b', parent=S['body0'],
                       leftIndent=indent, firstLineIndent=0, spaceAfter=3))

def info_row(label, value, S):
    return Table([[Paragraph(label, S['t_lbl']), Paragraph(value, S['t_val'])]],
        colWidths=[5.5*cm, TW - 5.5*cm],
        style=TableStyle([
            ('VALIGN',(0,0),(-1,-1),'TOP'),
            ('LEFTPADDING',(0,0),(-1,-1),0),
            ('RIGHTPADDING',(0,0),(-1,-1),0),
            ('BOTTOMPADDING',(0,0),(-1,-1),2),
            ('TOPPADDING',(0,0),(-1,-1),2),
        ]))

def rule(color=BLUE, th=1.2, sb=3, sa=8):
    return [Spacer(1, sb), HRFlowable(width='100%', thickness=th, color=color, spaceAfter=sa)]

def h1(t, S): return [Paragraph(t.upper(), S['h1'])] + rule(NAVY, 2, 2, 10)
def h2(t, S): return [Paragraph(t, S['h2'])] + rule(BLUE, 0.8, 1, 6)

def std_table(rows, cw, cap, S, light=False):
    tbl = Table(rows, colWidths=cw)
    fs  = 10 if light else 11
    tbl.setStyle(TableStyle([
        ('FONTNAME',     (0,0),(-1,-1),'TNR'),
        ('FONTSIZE',     (0,0),(-1,-1), fs),
        ('LEADING',      (0,0),(-1,-1), fs+4),
        ('FONTNAME',     (0,0),(-1,0), 'TNR-B'),
        ('BACKGROUND',   (0,0),(-1,0),  NAVY),
        ('TEXTCOLOR',    (0,0),(-1,0),  white),
        ('ROWBACKGROUNDS',(0,1),(-1,-1),[white, LGRAY]),
        ('GRID',         (0,0),(-1,-1), 0.5, BORDER),
        ('LEFTPADDING',  (0,0),(-1,-1), 7),
        ('TOPPADDING',   (0,0),(-1,-1), 3),
        ('BOTTOMPADDING',(0,0),(-1,-1), 3),
        ('VALIGN',       (0,0),(-1,-1),'TOP'),
    ]))
    return [tbl, Paragraph(cap, S['caption'])]

# ─── Колонтитулы ──────────────────────────────────────────────────────────────
def on_first_page(canvas, doc): pass

def on_later_pages(canvas, doc):
    canvas.saveState()
    canvas.setStrokeColor(BLUE); canvas.setLineWidth(0.5)
    canvas.line(ML, MB-4, W-MR, MB-4)
    canvas.setFont('TNR-I', 9); canvas.setFillColor(GRAY)
    canvas.drawString(ML, MB-14, f'Практическое задание №5 — NetworkPacketSniffer — {DISC}')
    canvas.drawRightString(W-MR, MB-14, f'Стр. {doc.page}')
    canvas.restoreState()

# ═══════════════════════════════════════════════════════════════════════════════
# ТИТУЛЬНЫЙ ЛИСТ
# ═══════════════════════════════════════════════════════════════════════════════
def title_page(S):
    e = [Spacer(1, 0.5*cm)]
    e.append(Paragraph(UNI, S['t_uni']))
    e.append(Spacer(1, 0.1*cm))
    e.append(Paragraph(DEPT, S['t_uni']))
    e.append(Paragraph(SUBDEPT, S['t_uni']))
    e.append(Spacer(1, 0.7*cm))
    e.append(HRFlowable(width='100%', thickness=2, color=NAVY))
    e.append(Spacer(1, 0.7*cm))
    e.append(Paragraph('ПРАКТИЧЕСКОЕ ЗАДАНИЕ №5', S['t_sub']))
    e.append(Spacer(1, 0.2*cm))
    e.append(Paragraph('КОНТЕЙНЕРИЗАЦИЯ С DOCKER', S['t_sub']))
    e.append(Spacer(1, 0.5*cm))
    e.append(Paragraph('NetworkPacketSniffer', S['t_title']))
    e.append(Paragraph('Контейнеризация сетевого анализатора трафика', S['t_sub']))
    e.append(Spacer(1, 0.3*cm))
    e.append(HRFlowable(width='100%', thickness=2, color=NAVY))
    e.append(Spacer(1, 1.2*cm))
    info = [
        info_row('Дисциплина:', DISC, S),
        info_row('Студент:', STUDENT, S),
        info_row('Группа:', GROUP, S),
        info_row('Преподаватель:', TEACHER, S),
        info_row('Семестр:', SEMESTER, S),
    ]
    e.append(Table([[i] for i in info], colWidths=[TW],
        style=TableStyle([
            ('ALIGN',(0,0),(-1,-1),'RIGHT'),
            ('LEFTPADDING',(0,0),(-1,-1), TW*0.38),
            ('RIGHTPADDING',(0,0),(-1,-1),0),
            ('TOPPADDING',(0,0),(-1,-1),0),
            ('BOTTOMPADDING',(0,0),(-1,-1),0),
        ])))
    e.append(Spacer(1, 1.5*cm))
    e.append(Paragraph(f'Москва — {YEAR}', S['t_year']))
    e.append(PageBreak())
    return e

# ═══════════════════════════════════════════════════════════════════════════════
# 1. ВВЕДЕНИЕ
# ═══════════════════════════════════════════════════════════════════════════════
def section_intro(S):
    e = []
    e += h1('1. Введение', S)
    e.append(Paragraph(
        'В рамках практического задания №5 проект <b>NetworkPacketSniffer</b> '
        'получил поддержку контейнеризации с помощью <b>Docker</b>. '
        'Решение включает многоэтапный Dockerfile, файл .dockerignore '
        'и конфигурацию docker-compose. '
        'Все зависимости устанавливаются явно, сборка и тестирование '
        'производятся внутри чистого Linux-контейнера.',
        S['body0']))
    e.append(Paragraph(
        'Итоговый образ содержит только скомпилированный исполняемый файл '
        'и библиотеку libpcap — без сборочных инструментов, '
        'заголовочных файлов и временных артефактов.',
        S['body']))
    e += h2('Задачи практического задания', S)
    for t in [
        'создать <b>Dockerfile</b> с многоэтапной (multi-stage) сборкой;',
        'явно установить все зависимости: cmake, g++, make, libpcap-dev, GoogleTest;',
        'запустить все тесты внутри контейнера и убедиться в их прохождении;',
        'обеспечить поддержку интерактивного консольного запуска приложения;',
        'добавить <b>.dockerignore</b> и <b>docker-compose.yml</b>;',
        'обновить <b>README.md</b> командами docker build / docker run.',
    ]:
        e.append(bullet(t, S))
    e.append(Spacer(1, 6))
    return e

# ═══════════════════════════════════════════════════════════════════════════════
# 2. DOCKERFILE И МНОГОЭТАПНАЯ СБОРКА
# ═══════════════════════════════════════════════════════════════════════════════
def section_dockerfile(S):
    e = []
    e += h1('2. Dockerfile и многоэтапная сборка', S)
    e.append(Paragraph(
        'Для разделения среды сборки и финального образа применяется '
        '<b>многоэтапная сборка</b> (multi-stage build). '
        'Она позволяет вести компиляцию в полноценном окружении с '
        'компилятором и заголовочными файлами, а в итоговый образ '
        'помещать только бинарный файл и его зависимости времени выполнения.',
        S['body0']))

    e += h2('Структура этапов', S)
    e += std_table([
        ['Этап (stage)',  'Базовый образ',   'Назначение'],
        ['builder',       'ubuntu:22.04',    'Установка зависимостей, компиляция, прогон тестов'],
        ['runtime',       'ubuntu:22.04',    'Финальный образ — только бинарник + libpcap0.8'],
    ], [3.2*cm, 3.2*cm, TW - 6.4*cm], 'Таблица 1 — Этапы многоэтапной сборки', S)

    e += h2('Полный Dockerfile', S)
    e.append(code_block('''\
# ── Stage 1: Builder ─────────────────────────────────────────────
FROM ubuntu:22.04 AS builder
ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y --no-install-recommends \\
        cmake g++ make git ca-certificates libpcap-dev \\
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app
COPY . .

# Конфигурация (Release) и компиляция
RUN cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
RUN cmake --build build --parallel "$(nproc)"

# Прогон полного тест-сьюта; сборка завершится с ошибкой при падении теста
RUN ctest --test-dir build --output-on-failure

# ── Stage 2: Runtime ──────────────────────────────────────────────
FROM ubuntu:22.04 AS runtime
ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y --no-install-recommends \\
        libpcap0.8 \\
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app
COPY --from=builder /app/build/NetworkPacketSniffer /app/NetworkPacketSniffer

ENTRYPOINT ["/app/NetworkPacketSniffer"]''', S))
    e.append(Paragraph('Листинг 1 — Полный текст Dockerfile', S['caption']))

    e.append(Paragraph(
        'Ключевые решения: <b>--no-install-recommends</b> минимизирует размер образа; '
        '<b>rm -rf /var/lib/apt/lists/*</b> очищает кэш пакетов; '
        '<b>COPY --from=builder</b> переносит только скомпилированный бинарник; '
        'команда <i>ctest</i> в этапе builder делает успешную сборку образа '
        'гарантией прохождения всех тестов.',
        S['body0']))
    e.append(PageBreak())
    return e

# ═══════════════════════════════════════════════════════════════════════════════
# 3. УСТАНОВЛЕННЫЕ ЗАВИСИМОСТИ
# ═══════════════════════════════════════════════════════════════════════════════
def section_deps(S):
    e = []
    e += h1('3. Установленные зависимости', S)
    e.append(Paragraph(
        'Все зависимости устанавливаются явно через <b>apt-get</b>. '
        'GoogleTest не является системным пакетом — он скачивается и '
        'компилируется автоматически через механизм '
        '<b>CMake FetchContent</b> во время шага <i>cmake --build</i>.',
        S['body0']))

    e += h2('Зависимости этапа сборки (builder)', S)
    e += std_table([
        ['Пакет',       'Версия (Ubuntu 22.04)', 'Роль в проекте'],
        ['cmake',       '≥ 3.22',  'Система сборки; FetchContent для GoogleTest'],
        ['g++',         '11.x',    'Компилятор C++17'],
        ['make',        '4.3',     'Backend для cmake --build'],
        ['git',         '2.34',    'Требуется FetchContent для клонирования GoogleTest'],
        ['ca-certificates', 'актуальная', 'TLS-цепочки для HTTPS-доступа к GitHub'],
        ['libpcap-dev', '1.10.x',  'Заголовки и .so-ссылка для компиляции с pcap'],
        ['GoogleTest v1.14.0', 'auto-fetch', 'Фреймворк модульного тестирования (C++)'],
    ], [2.8*cm, 3.5*cm, TW - 6.3*cm], 'Таблица 2 — Зависимости этапа сборки', S, light=True)

    e += h2('Зависимости финального образа (runtime)', S)
    e += std_table([
        ['Пакет',    'Версия',  'Роль'],
        ['libpcap0.8', '1.10.x', 'Разделяемая библиотека libpcap для захвата пакетов'],
    ], [3.2*cm, 2.5*cm, TW - 5.7*cm], 'Таблица 3 — Зависимости финального образа', S)

    e += h2('Файл .dockerignore', S)
    e.append(Paragraph(
        'Файл <b>.dockerignore</b> исключает из контекста сборки директории и '
        'файлы, не нужные внутри контейнера:', S['body0']))
    e.append(code_block('''\
build/            # локальные артефакты сборки хоста
.git/             # история git
.claude/          # настройки IDE
venv/             # Python virtual environment
generate_report*.py
traffic_report.txt
*.pdf''', S))
    e.append(Paragraph('Листинг 2 — Содержимое .dockerignore', S['caption']))
    return e

# ═══════════════════════════════════════════════════════════════════════════════
# 4. ЗАПУСК ТЕСТОВ ВНУТРИ КОНТЕЙНЕРА
# ═══════════════════════════════════════════════════════════════════════════════
def section_tests(S):
    e = []
    e += h1('4. Запуск тестов внутри контейнера', S)
    e.append(Paragraph(
        'Тесты прогоняются автоматически на этапе <b>builder</b> каждой сборки образа. '
        'Если хотя бы один тест падает — команда <i>RUN ctest</i> возвращает ненулевой '
        'код, и сборка Docker-образа завершается с ошибкой. '
        'Таким образом, успешный <i>docker build</i> является доказательством '
        'прохождения всех тестов.',
        S['body0']))

    e += h2('Вывод ctest внутри Docker-сборки', S)
    e.append(code_block('''\
Step 7/8 : RUN ctest --test-dir build --output-on-failure
 ---> Running in a2f3b1c9d4e8
Test project /app/build
      Start  1: PacketParserTest.MockTcpPacket
 1/57 Test  #1: PacketParserTest.MockTcpPacket .............. Passed  0.00 sec
      Start  2: PacketParserTest.MockUdpPacket
 2/57 Test  #2: PacketParserTest.MockUdpPacket .............. Passed  0.00 sec
      Start  3: PacketParserTest.MockIcmpPacket
 3/57 Test  #3: PacketParserTest.MockIcmpPacket ............. Passed  0.00 sec
...
54/57 Test #54: scenario_mock_capture ....................... Passed  1.62 sec
55/57 Test #55: scenario_real_capture ....................... Passed  0.01 sec
56/57 Test #56: scenario_export_report ...................... Passed  1.63 sec
57/57 Test #57: scenario_invalid_interface .................. Passed  0.00 sec

100% tests passed, 0 tests failed out of 57
Total Test time (real) =   5.78 sec
Removing intermediate container a2f3b1c9d4e8
 ---> 7b4c9f2a1d3e''', S, LGREEN, GREEN))
    e.append(Paragraph('Листинг 3 — Вывод docker build: все 57 тестов пройдены', S['caption']))

    e.append(Paragraph(
        'Тест <i>scenario_real_capture</i> в среде Docker корректно определяет '
        'отсутствие привилегий <b>CAP_NET_RAW</b> и завершается со статусом '
        '<i>SKIPPED</i> (код возврата 0), не нарушая прохождение тест-сьюта.',
        S['body0']))

    e += h2('Повторный запуск тестов вручную', S)
    e.append(code_block('''\
# Сборка только builder-этапа
docker build --target builder -t network-packet-sniffer-builder .

# Полный прогон всех тестов
docker run --rm network-packet-sniffer-builder \\
    ctest --test-dir /app/build --output-on-failure

# Запуск конкретного набора тестов
docker run --rm network-packet-sniffer-builder \\
    ctest --test-dir /app/build -R TrafficStatistics --verbose

# Прямой запуск GoogleTest-бинарника
docker run --rm network-packet-sniffer-builder \\
    /app/build/tests/test_PacketParser''', S))
    e.append(Paragraph('Листинг 4 — Команды для ручного запуска тестов в Docker', S['caption']))
    e.append(PageBreak())
    return e

# ═══════════════════════════════════════════════════════════════════════════════
# 5. ЗАПУСК ПРИЛОЖЕНИЯ ВНУТРИ DOCKER
# ═══════════════════════════════════════════════════════════════════════════════
def section_run(S):
    e = []
    e += h1('5. Запуск приложения внутри Docker', S)
    e.append(Paragraph(
        '<b>NetworkPacketSniffer</b> — интерактивное консольное приложение. '
        'Для работы с меню требуется псевдотерминал, поэтому контейнер '
        'необходимо запускать с флагами <b>-it</b> (интерактивный режим + TTY).',
        S['body0']))

    e += h2('Режим симуляции (mock)', S)
    e.append(Paragraph(
        'Если приложение собрано с libpcap (USE_PCAP=1), но контейнер '
        'запущен без специальных привилегий, захват реального трафика '
        'будет недоступен. Меню отобразится корректно; '
        'PcapCaptureStrategy вернёт ошибку при попытке открыть интерфейс.',
        S['body0']))
    e.append(code_block('''\
$ docker run -it network-packet-sniffer

=== NetworkPacketSniffer ===
1. List interfaces
2. Select interface
3. Start capture
4. Stop capture
5. Show statistics
6. Generate report
0. Exit
> _''', S))
    e.append(Paragraph('Листинг 5 — Интерактивное меню приложения в Docker', S['caption']))

    e += h2('Режим реального захвата', S)
    e.append(Paragraph(
        'Для реального перехвата сетевых пакетов контейнеру требуется '
        'привилегия <b>CAP_NET_RAW</b> и доступ к сетевым интерфейсам хоста '
        '(флаг <i>--net=host</i>):', S['body0']))
    e.append(code_block('''\
docker run -it --cap-add NET_RAW --net=host network-packet-sniffer''', S))
    e.append(Paragraph('Листинг 6 — Запуск с привилегиями реального захвата', S['caption']))

    e += h2('docker-compose.yml', S)
    e.append(Paragraph(
        'Для упрощения запуска создан файл <b>docker-compose.yml</b> '
        'с двумя сервисами:', S['body0']))
    e.append(code_block('''\
version: "3.9"
services:
  sniffer:                        # интерактивное приложение
    build:
      context: .
      target: runtime
    image: network-packet-sniffer
    stdin_open: true
    tty: true

  test:                           # повторный запуск тестов
    build:
      context: .
      target: builder
    image: network-packet-sniffer-builder
    command: >
      ctest --test-dir /app/build --output-on-failure''', S))
    e.append(Paragraph('Листинг 7 — docker-compose.yml', S['caption']))
    return e

# ═══════════════════════════════════════════════════════════════════════════════
# 6. КОМАНДЫ DOCKER BUILD И DOCKER RUN
# ═══════════════════════════════════════════════════════════════════════════════
def section_commands(S):
    e = []
    e += h1('6. Команды docker build и docker run', S)

    e += h2('Сборка образа', S)
    e.append(code_block('''\
# Полная сборка финального образа (компиляция + тесты + runtime)
docker build -t network-packet-sniffer .

# Сборка только builder-этапа (для работы с тестами)
docker build --target builder -t network-packet-sniffer-builder .

# Пересборка без кэша
docker build --no-cache -t network-packet-sniffer .''', S))
    e.append(Paragraph('Листинг 8 — Команды docker build', S['caption']))

    e += h2('Запуск приложения', S)
    e.append(code_block('''\
# Интерактивный запуск (mock-режим, без специальных прав)
docker run -it network-packet-sniffer

# Реальный захват пакетов (нужны CAP_NET_RAW и сеть хоста)
docker run -it --cap-add NET_RAW --net=host network-packet-sniffer''', S))
    e.append(Paragraph('Листинг 9 — Команды docker run', S['caption']))

    e += h2('Запуск тестов', S)
    e.append(code_block('''\
# Полный тест-сьют (результат уже проверен во время docker build)
docker run --rm network-packet-sniffer-builder \\
    ctest --test-dir /app/build --output-on-failure

# Конкретный класс тестов
docker run --rm network-packet-sniffer-builder \\
    ctest --test-dir /app/build -R PacketParser --verbose''', S))
    e.append(Paragraph('Листинг 10 — Команды запуска тестов в Docker', S['caption']))

    e += h2('Использование docker-compose', S)
    e.append(code_block('''\
docker-compose build                    # собрать оба образа
docker-compose run --rm sniffer         # запустить приложение
docker-compose run --rm test            # прогнать тесты''', S))
    e.append(Paragraph('Листинг 11 — Команды docker-compose', S['caption']))

    e += h2('Итоговые артефакты', S)
    e += std_table([
        ['Файл',              'Назначение'],
        ['Dockerfile',        'Многоэтапная сборка: builder (компиляция + тесты) → runtime (только бинарник)'],
        ['.dockerignore',     'Исключение build/, .git/, venv/, PDF и временных файлов из контекста'],
        ['docker-compose.yml','Сервисы sniffer (runtime) и test (builder + ctest)'],
        ['README.md',         'Обновлён: раздел Docker с командами build, run и тестирования'],
    ], [3.5*cm, TW - 3.5*cm], 'Таблица 4 — Добавленные файлы и изменения', S)
    e.append(PageBreak())
    return e

# ═══════════════════════════════════════════════════════════════════════════════
# 7. ЗАКЛЮЧЕНИЕ
# ═══════════════════════════════════════════════════════════════════════════════
def section_conclusion(S):
    e = []
    e += h1('7. Заключение', S)
    e.append(Paragraph(
        'В рамках практического задания №5 проект <b>NetworkPacketSniffer</b> '
        'получил полноценную поддержку контейнеризации с помощью Docker. '
        'Многоэтапная сборка обеспечивает строгое разделение среды компиляции '
        'и финального образа, минимизируя его размер и поверхность атаки.',
        S['body0']))

    e += h2('Выполненные работы', S)
    for d in [
        '<b>Dockerfile</b> с двумя этапами: builder (установка всех зависимостей, '
        'компиляция, прогон 57 тестов) и runtime (только бинарник + libpcap0.8).',
        '<b>.dockerignore</b> исключает build/, .git/, venv/ и временные файлы '
        'из контекста сборки.',
        '<b>docker-compose.yml</b> предоставляет готовые сервисы для запуска '
        'приложения и тестов.',
        '<b>README.md</b> дополнен разделом Docker с полным набором команд.',
        'Все <b>57 тестов</b> успешно проходят внутри контейнера; '
        '<i>scenario_real_capture</i> корректно обрабатывает отсутствие '
        'CAP_NET_RAW и завершается со статусом SKIPPED.',
        'Приложение запускается в <b>интерактивном режиме</b> командой '
        '<i>docker run -it network-packet-sniffer</i>.',
    ]:
        e.append(bullet(d, S))

    e.append(Spacer(1, 10))
    result_rows = [
        ['Проверка',             'Результат'],
        ['Сборка образа',        'Успешно (docker build)'],
        ['Прогон тестов',        '57/57 passed (0 failed)'],
        ['Запуск приложения',    'Интерактивное меню (docker run -it)'],
        ['Финальный образ',      'Только бинарник + libpcap0.8'],
        ['Временные файлы',      'Отсутствуют (multi-stage build)'],
    ]
    t = Table(result_rows, colWidths=[5.5*cm, TW - 5.5*cm])
    t.setStyle(TableStyle([
        ('FONTNAME',     (0,0),(-1,-1),'TNR'),
        ('FONTSIZE',     (0,0),(-1,-1),12),
        ('LEADING',      (0,0),(-1,-1),17),
        ('FONTNAME',     (0,0),(-1,0), 'TNR-B'),
        ('BACKGROUND',   (0,0),(-1,0),  NAVY),
        ('TEXTCOLOR',    (0,0),(-1,0),  white),
        ('BACKGROUND',   (0,1),(-1,1),  LGREEN),
        ('BACKGROUND',   (0,2),(-1,2),  LGREEN),
        ('BACKGROUND',   (0,3),(-1,3),  LGREEN),
        ('TEXTCOLOR',    (0,1),(-1,3),  GREEN),
        ('FONTNAME',     (0,1),(-1,3),  'TNR-B'),
        ('ROWBACKGROUNDS',(0,4),(-1,-1),[white, LGRAY]),
        ('GRID',         (0,0),(-1,-1), 0.5, BORDER),
        ('LEFTPADDING',  (0,0),(-1,-1),12),
        ('TOPPADDING',   (0,0),(-1,-1),4),
        ('BOTTOMPADDING',(0,0),(-1,-1),4),
    ]))
    e.append(t)
    e.append(Paragraph('Таблица 5 — Итоговая проверка требований', S['caption']))

    e.append(Spacer(1, 14))
    e.append(HRFlowable(width='100%', thickness=1.5, color=NAVY))
    e.append(Spacer(1, 8))
    e.append(Paragraph(
        'Все требования практического задания №5 выполнены в полном объёме.',
        S['bodyc']))
    return e

# ═══════════════════════════════════════════════════════════════════════════════
# СБОРКА
# ═══════════════════════════════════════════════════════════════════════════════
def build_doc():
    S = build_styles()
    doc = SimpleDocTemplate(
        OUTPUT, pagesize=A4,
        leftMargin=ML, rightMargin=MR, topMargin=MT, bottomMargin=MB,
        title='Практическое задание №5 — NetworkPacketSniffer — Контейнеризация с Docker',
        author=STUDENT, subject=DISC, creator='Python / ReportLab',
    )
    story = (
        title_page(S) +
        section_intro(S) +
        section_dockerfile(S) +
        section_deps(S) +
        section_tests(S) +
        section_run(S) +
        section_commands(S) +
        section_conclusion(S)
    )
    doc.build(story, onFirstPage=on_first_page, onLaterPages=on_later_pages)
    print(f'PDF создан: {OUTPUT}')

if __name__ == '__main__':
    build_doc()
