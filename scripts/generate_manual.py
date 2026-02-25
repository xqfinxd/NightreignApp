#!/usr/bin/env python3

import csvutil
import defines

paths = defines.PathDefinitions(__file__)

map_cpp_header_file = paths.get_cpp_header("ManualMapRow")
map_header = {
    'id': 'int',
    'name': 'std::string',
}
csvutil.generate_cpp_header(map_header, map_cpp_header_file, 'ManualMapRow')

nightlord_cpp_header_file = paths.get_cpp_header("ManualNightlordRow")
nightlord_header = {
    'id': 'int',
    'name': 'std::string',
}
csvutil.generate_cpp_header(nightlord_header, nightlord_cpp_header_file, 'ManualNightlordRow')

validation_cpp_header_file = paths.get_cpp_header("ManualValidationRow")
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

spot_cpp_header_file = paths.get_cpp_header("ManualSpotRow")
spot_header = {
    'id': 'int',
    'disable_filter': 'int',
    'disable_view': 'int',
}
csvutil.generate_cpp_header(spot_header, spot_cpp_header_file, 'ManualSpotRow')

spot_label_cpp_header_file = paths.get_cpp_header("ManualSpotLabelRow")
spot_label_header = {
    'id': 'int',
    'direction': 'int',
    'offsetx': 'float',
    'offsety': 'float',
    'showicon': 'int',
}
csvutil.generate_cpp_header(spot_label_header, spot_label_cpp_header_file, 'ManualSpotLabelRow')

grid_cpp_header_file = paths.get_cpp_header("ManualGridRow")
grid_header = {
    'x': 'int',
    'y': 'int',
    'height': 'float',
    'map': 'int',
}
csvutil.generate_cpp_header(grid_header, grid_cpp_header_file, 'ManualGridRow')