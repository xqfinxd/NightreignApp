INSERT OR IGNORE INTO Starter (starter_id, grid_x, grid_z, pos_x, pos_z) VALUES
(13000,42,36,28.44,88.18),
(13002,45,39,-29.87,-76.8),
(13001,43,36,58.74,-64.57),
(700,42,36,-74.0,62.0),
(701,42,37,-66.0,44.0),
(702,42,38,-48.0,98.46),
(703,43,38,-86.0,32.0),
(704,44,36,-22.0,-72.0),
(705,44,37,-52.0,-76.0),
(706,44,39,-72.0,44.0),
(707,45,37,86.0,4.0),
(708,45,38,-86.0,81.0)
ON CONFLICT(starter_id) DO UPDATE SET
    grid_x=excluded.grid_x,
    grid_z=excluded.grid_z,
    pos_x=excluded.pos_x,
    pos_z=excluded.pos_z;