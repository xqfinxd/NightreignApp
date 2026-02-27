CREATE TABLE IF NOT EXISTS Map (
    map_id TINYINT PRIMARY KEY,
    name CHAR(8) NOT NULL
);

CREATE TABLE IF NOT EXISTS Nightlord (
    nightlord_id TINYINT PRIMARY KEY,
    name CHAR(32) NOT NULL
);

CREATE TABLE IF NOT EXISTS SmallBaseMap (
    smallbase_id INT PRIMARY KEY,
    label CHAR(32) NOT NULL,
    icon_atlas CHAR(32),
    icon_scale DECIMAL(6,2) DEFAULT 1.0,
    group_id TINYINT,
    flags TINYINT DEFAULT 0
);

CREATE TABLE IF NOT EXISTS VariationParam (
    smallbase_id INT NOT NULL,
    variation_id TINYINT NOT NULL,
    label CHAR(32) NOT NULL,
    icon_atlas CHAR(32),
    PRIMARY KEY (smallbase_id, variation_id),
    FOREIGN KEY (smallbase_id) REFERENCES SmallBaseMap(smallbase_id) ON DELETE CASCADE
);

CREATE TABLE IF NOT EXISTS Starter (
    starter_id SMALLINT PRIMARY KEY,
    grid_x TINYINT,
    grid_z TINYINT,
    pos_x DECIMAL(6,2),
    pos_z DECIMAL(6,2),
    height DECIMAL(6,2) DEFAULT 0.0
);

CREATE TABLE IF NOT EXISTS PlayArea (
    playarea_id SMALLINT PRIMARY KEY,
    grid_x TINYINT,
    grid_z TINYINT,
    pos_x DECIMAL(6,2),
    pos_z DECIMAL(6,2),
    height DECIMAL(6,2) DEFAULT 0.0
);

CREATE TABLE IF NOT EXISTS AttachPoint (
    attach_id INT PRIMARY KEY,
    grid_x TINYINT,
    grid_z TINYINT,
    pos_x DECIMAL(6,2),
    pos_z DECIMAL(6,2),
    height DECIMAL(6,2) DEFAULT 0.0
);

CREATE TABLE IF NOT EXISTS Pattern (
    pattern_id SMALLINT PRIMARY KEY,
    nightlord_id TINYINT NOT NULL,
    dlc TINYINT DEFAULT 0,
    map_id TINYINT NOT NULL,
    
    day1boss_smallbase_id SMALLINT,
    day2boss_smallbase_id SMALLINT,
    
    day1extraboss_smallbase_id SMALLINT,
    day2extraboss_smallbase_id SMALLINT,
    
    starter_id SMALLINT,
    
    day1_playarea_id SMALLINT,
    day2_playarea_id SMALLINT,
    
    FOREIGN KEY (nightlord_id) REFERENCES Nightlord(nightlord_id),
    FOREIGN KEY (map_id) REFERENCES Map(map_id),
    FOREIGN KEY (day1boss_smallbase_id) REFERENCES SmallBaseMap(smallbase_id),
    FOREIGN KEY (day2boss_smallbase_id) REFERENCES SmallBaseMap(smallbase_id),
    FOREIGN KEY (day1extraboss_smallbase_id) REFERENCES SmallBaseMap(smallbase_id),
    FOREIGN KEY (day2extraboss_smallbase_id) REFERENCES SmallBaseMap(smallbase_id),
    FOREIGN KEY (starter_id) REFERENCES Starter(starter_id),
    FOREIGN KEY (day1_playarea_id) REFERENCES PlayArea(playarea_id),
    FOREIGN KEY (day2_playarea_id) REFERENCES PlayArea(playarea_id)
);

CREATE TABLE IF NOT EXISTS SpotConfig (
    attach_id INT NOT NULL,
    pattern_id SMALLINT NOT NULL,
    smallbase_id INT NOT NULL,
    variation_id TINYINT NOT NULL,
    
    PRIMARY KEY (attach_id, pattern_id, smallbase_id, variation_id),
    
    FOREIGN KEY (attach_id) REFERENCES AttachPoint(attach_id) ON DELETE CASCADE,
    FOREIGN KEY (pattern_id) REFERENCES Pattern(pattern_id) ON DELETE CASCADE,
    FOREIGN KEY (smallbase_id, variation_id) REFERENCES VariationParam(smallbase_id, variation_id) ON DELETE CASCADE
);

CREATE INDEX IF NOT EXISTS idx_pattern_nightlord ON Pattern(nightlord_id);
CREATE INDEX IF NOT EXISTS idx_pattern_map ON Pattern(map_id);
CREATE INDEX IF NOT EXISTS idx_pattern_starter ON Pattern(starter_id);

CREATE INDEX IF NOT EXISTS idx_variation_smallbase ON VariationParam(smallbase_id);

CREATE INDEX IF NOT EXISTS idx_spotconfig_attach ON SpotConfig(attach_id);
CREATE INDEX IF NOT EXISTS idx_spotconfig_pattern ON SpotConfig(pattern_id);
CREATE INDEX IF NOT EXISTS idx_spotconfig_smallbase ON SpotConfig(smallbase_id);

CREATE INDEX IF NOT EXISTS idx_attach_grid_pos ON AttachPoint(grid_x, grid_z, pos_x, pos_z);
CREATE INDEX IF NOT EXISTS idx_attach_grid_height ON AttachPoint(grid_x, grid_z, height);

CREATE INDEX IF NOT EXISTS idx_smallbase_group ON SmallBaseMap(group_id);