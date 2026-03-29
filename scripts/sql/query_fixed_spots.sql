SELECT 
    ap.grid_x,
    ap.grid_z,
    ap.pos_x,
    ap.pos_z,
    p.map_id,
    p.dlc,
    COUNT(DISTINCT p.pattern_id) AS coord_pattern_count,
    (
        SELECT COUNT(*) 
        FROM Pattern 
        WHERE map_id = p.map_id AND dlc = p.dlc
    ) AS map_total_patterns_by_dlc,
    ROUND(
        COUNT(DISTINCT p.pattern_id) * 1.0 / 
        (SELECT COUNT(*) FROM Pattern WHERE map_id = p.map_id AND dlc = p.dlc),
        4
    ) AS occupancy_rate,
    ROUND(
        COUNT(DISTINCT p.pattern_id) * 100.0 / 
        (SELECT COUNT(*) FROM Pattern WHERE map_id = p.map_id AND dlc = p.dlc),
        2
    ) || '%' AS occupancy_percentage,
    GROUP_CONCAT(DISTINCT ap.attach_id) AS attach_ids
FROM AttachPoint ap
JOIN SpotConfig sc ON ap.attach_id = sc.attach_id
JOIN Pattern p ON sc.pattern_id = p.pattern_id
GROUP BY ap.grid_x, ap.grid_z, ap.pos_x, ap.pos_z, p.map_id, p.dlc
ORDER BY occupancy_rate DESC;