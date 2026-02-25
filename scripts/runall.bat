@echo off
set "target_dir=..\src\generated"

if /i "%1"=="clean" (
    if exist "%target_dir%" (
        rmdir /s /q "%target_dir%"
    )
    mkdir "%target_dir%"
)

python -B .\generate_manifest.py
python -B .\generate_atlas.py
python -B .\generate_pattern_list.py
python -B .\generate_starter_list.py
python -B .\generate_spot_list.py
python -B .\generate_manual.py
python -B .\generate_events.py
python -B .\generate_chinese_chars.py
python -B .\generate_font_subset.py

pause