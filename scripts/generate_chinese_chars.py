#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Extract Chinese characters from manual CSV files and CHS macro in source code
"""
import os
import re
import csv
from pathlib import Path

def is_chinese_char(char):
    """Check if a character is Chinese"""
    return '\u4e00' <= char <= '\u9fff'

def extract_chinese_from_text(text):
    """Extract all Chinese characters from text"""
    if not text:
        return set()
    return {char for char in text if is_chinese_char(char)}

def extract_from_csv_files(asset_dir):
    """Extract Chinese characters from manual*.csv files"""
    chinese_chars = set()
    
    # Search for manual*.csv in datas directory
    search_dirs = [
        asset_dir / 'assets' / 'datas'
    ]
    
    for search_dir in search_dirs:
        if not search_dir.exists():
            continue
            
        for csv_file in search_dir.glob('manual*.csv'):
            print(f"Processing CSV: {csv_file}")
            try:
                with open(csv_file, 'r', encoding='utf-8') as f:
                    # Read all content
                    content = f.read()
                    chars = extract_chinese_from_text(content)
                    chinese_chars.update(chars)
                    print(f"  Found {len(chars)} unique Chinese characters")
            except Exception as e:
                print(f"  Error reading {csv_file}: {e}")
    
    return chinese_chars

def extract_from_source_code(src_dir):
    """Extract Chinese characters from CHS() macros in source code"""
    chinese_chars = set()
    
    if not src_dir.exists():
        print(f"Source directory not found: {src_dir}")
        return chinese_chars
    
    # Pattern to match CHS("...") or CHS('...')
    chs_pattern = re.compile(r'CHS\s*\(\s*["\']([^"\']+)["\']\s*\)', re.UNICODE)
    
    for cpp_file in src_dir.rglob('*.cpp'):
        try:
            with open(cpp_file, 'r', encoding='utf-8') as f:
                content = f.read()
                matches = chs_pattern.findall(content)
                
                if matches:
                    print(f"Processing source: {cpp_file}")
                    for match in matches:
                        chars = extract_chinese_from_text(match)
                        chinese_chars.update(chars)
                        if chars:
                            print(f"  Found CHS(\"{match}\") with {len(chars)} Chinese chars")
        except Exception as e:
            print(f"  Error reading {cpp_file}: {e}")
    
    for h_file in src_dir.rglob('*.h'):
        try:
            with open(h_file, 'r', encoding='utf-8') as f:
                content = f.read()
                matches = chs_pattern.findall(content)
                
                if matches:
                    print(f"Processing header: {h_file}")
                    for match in matches:
                        chars = extract_chinese_from_text(match)
                        chinese_chars.update(chars)
                        if chars:
                            print(f"  Found CHS(\"{match}\") with {len(chars)} Chinese chars")
        except Exception as e:
            print(f"  Error reading {h_file}: {e}")
    
    return chinese_chars

def main():
    # Get project root directory (parent of scripts)
    base_dir = Path(__file__).resolve().parent.parent
    
    print(f"Project root: {base_dir}")
    print("=" * 60)
    
    # Extract from CSV files
    print("\n--- Extracting from CSV files ---")
    csv_chars = extract_from_csv_files(base_dir / 'nightreign')
    print(f"\nTotal Chinese characters from CSV: {len(csv_chars)}")
    
    # Extract from source code
    print("\n--- Extracting from source code ---")
    code_chars = extract_from_source_code(base_dir / 'src')
    print(f"\nTotal Chinese characters from code: {len(code_chars)}")
    
    # Combine all characters
    all_chars = csv_chars | code_chars
    print(f"\n--- Combined Results ---")
    print(f"Total unique Chinese characters: {len(all_chars)}")
    
    # Sort characters for consistent output
    sorted_chars = sorted(all_chars)
    
    # Write to chs.txt
    output_file = base_dir / 'nightreign' / 'assets' / 'datas' / 'chs.txt'
    output_file.parent.mkdir(parents=True, exist_ok=True)
    
    with open(output_file, 'w', encoding='utf-8') as f:
        f.write(''.join(sorted_chars))
    
    print(f"\nChinese characters written to: {output_file}")
    print(f"Character preview (first 50): {''.join(sorted_chars[:50])}")
    
    # Print statistics
    print(f"\n--- Statistics ---")
    print(f"CSV characters: {len(csv_chars)}")
    print(f"Code characters: {len(code_chars)}")
    print(f"Total unique: {len(all_chars)}")
    print(f"Estimated font memory savings: ~{(20902 - len(all_chars)) * 100 / 20902:.1f}%")
    print("(Compared to loading all common simplified Chinese characters)")

if __name__ == '__main__':
    main()
