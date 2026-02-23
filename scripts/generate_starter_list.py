#!/usr/bin/env python3

import csvutil
from pathlib import Path

starter_distribution = [
    { 'id':0, 'starter':700},
    { 'id':0, 'starter':701},
    { 'id':0, 'starter':702},
    { 'id':0, 'starter':703},
    { 'id':0, 'starter':704},
    { 'id':0, 'starter':705},
    { 'id':0, 'starter':706},
    { 'id':0, 'starter':707},
    { 'id':0, 'starter':708},
    { 'id':1, 'starter':700},
    { 'id':1, 'starter':701},
    { 'id':1, 'starter':704},
    { 'id':1, 'starter':705},
    { 'id':1, 'starter':706},
    { 'id':1, 'starter':707},
    { 'id':1, 'starter':708},
    { 'id':2, 'starter':700},
    { 'id':2, 'starter':701},
    { 'id':2, 'starter':702},
    { 'id':2, 'starter':704},
    { 'id':2, 'starter':705},
    { 'id':2, 'starter':707},
    { 'id':2, 'starter':708},
    { 'id':3, 'starter':700},
    { 'id':3, 'starter':701},
    { 'id':3, 'starter':702},
    { 'id':3, 'starter':703},
    { 'id':3, 'starter':706},
    { 'id':3, 'starter':708},
    { 'id':4, 'starter':13002},
    { 'id':4, 'starter':13000},
    { 'id':4, 'starter':13001},
    { 'id':5, 'starter':702},
    { 'id':5, 'starter':703},
    { 'id':5, 'starter':704},
    { 'id':5, 'starter':705},
    { 'id':5, 'starter':706},
    { 'id':5, 'starter':707},
    { 'id':5, 'starter':708},
]

starter_list = [
    { 'id':13000, 'gridXNo':42, 'gridZNo':36, 'posX':28.44, 'posZ':88.18, 'height':0},
    { 'id':13002, 'gridXNo':45, 'gridZNo':39, 'posX':-29.87, 'posZ':-76.80, 'height':0},
    { 'id':13001, 'gridXNo':43, 'gridZNo':36, 'posX':58.74, 'posZ':-64.57, 'height':0},
    { 'id':700, 'gridXNo':42, 'gridZNo':36, 'posX':-74.00, 'posZ':62.00, 'height':0},
    { 'id':701, 'gridXNo':42, 'gridZNo':37, 'posX':-66.00, 'posZ':44.00, 'height':0},
    { 'id':702, 'gridXNo':42, 'gridZNo':38, 'posX':-48.00, 'posZ':98.46, 'height':0},
    { 'id':703, 'gridXNo':43, 'gridZNo':38, 'posX':-86.00, 'posZ':32.00, 'height':0},
    { 'id':704, 'gridXNo':44, 'gridZNo':36, 'posX':-22.00, 'posZ':-72.00, 'height':0},
    { 'id':705, 'gridXNo':44, 'gridZNo':37, 'posX':-52.00, 'posZ':-76.00, 'height':0},
    { 'id':706, 'gridXNo':44, 'gridZNo':39, 'posX':-72.00, 'posZ':44.00, 'height':0},
    { 'id':707, 'gridXNo':45, 'gridZNo':37, 'posX':86.00, 'posZ':4.00, 'height':0},
    { 'id':708, 'gridXNo':45, 'gridZNo':38, 'posX':-86.00, 'posZ':81.00, 'height':0},
]

base_dir = Path(__file__).resolve().parent.parent 
assets_dir = base_dir / "nightreign" / "assets"

list_header = {
    'id': 'int',
    'gridXNo': 'int',
    'gridZNo': 'int',
    'posX': 'float',
    'posZ': 'float',
    'height': 'float'
}
list_csv_file = assets_dir / "datas" / "autogen_starter_list.csv"
list_cpp_header_file = base_dir / "src" / "generated" / "StarterRow.h"
csvutil.generate_csv(list_header, starter_list, list_csv_file)
csvutil.generate_cpp_header(list_header, list_cpp_header_file, "StarterRow")

distribution_header = {
    'id': 'int',
    'starter': 'int'
}
distribution_csv_file = assets_dir / "datas" / "autogen_starter_distribution.csv"
distribution_cpp_header_file = base_dir / "src" / "generated" / "StarterDistributionRow.h"
csvutil.generate_csv(distribution_header, starter_distribution, distribution_csv_file)
csvutil.generate_cpp_header(distribution_header, distribution_cpp_header_file, "StarterDistributionRow")
