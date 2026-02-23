#!/usr/bin/env python3

import csvutil
from pathlib import Path

base_dir = Path(__file__).resolve().parent.parent 

map_cpp_header_file = base_dir / "src" / "generated" / "ManualMapRow.h"
map_header = {
    'id': 'int',
    'name': 'std::string',
}
csvutil.generate_cpp_header(map_header, map_cpp_header_file, 'ManualMapRow')

nightlord_cpp_header_file = base_dir / "src" / "generated" / "ManualNightlordRow.h"
nightlord_header = {
    'id': 'int',
    'name': 'std::string',
}
csvutil.generate_cpp_header(nightlord_header, nightlord_cpp_header_file, 'ManualNightlordRow')

validation_cpp_header_file = base_dir / "src" / "generated" / "ManualValidationRow.h"
validation_header = {
    'id': 'int',
    'smallBaseMapId': 'int',
    'variationId': 'int',
    'label': 'std::string',
    'sublabel': 'std::string',
    'icon': 'std::string',
    'visible': 'int',
    'iconScale': 'float',
}
csvutil.generate_cpp_header(validation_header, validation_cpp_header_file, 'ManualValidationRow')

spot_cpp_header_file = base_dir / "src" / "generated" / "ManualSpotRow.h"
spot_header = {
    'id': 'int',
    'disable_filter': 'int',
    'disable_view': 'int',
}
csvutil.generate_cpp_header(spot_header, spot_cpp_header_file, 'ManualSpotRow')

spot_label_cpp_header_file = base_dir / "src" / "generated" / "ManualSpotLabelRow.h"
spot_label_header = {
    'id': 'int',
    'direction': 'int',
    'offsetx': 'float',
    'offsety': 'float',
    'showicon': 'int',
}
csvutil.generate_cpp_header(spot_label_header, spot_label_cpp_header_file, 'ManualSpotLabelRow')

grid_cpp_header_file = base_dir / "src" / "generated" / "ManualGridRow.h"
grid_header = {
    'x': 'int',
    'y': 'int',
    'height': 'float',
    'map': 'int',
}
csvutil.generate_cpp_header(grid_header, grid_cpp_header_file, 'ManualGridRow')