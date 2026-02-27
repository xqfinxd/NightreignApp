INSERT OR IGNORE INTO Map (map_id, name) VALUES
(0,"普通"),
(1,"雪山"),
(2,"火山"),
(3,"腐败"),
(4,"大空洞"),
(5,"隐城")
ON CONFLICT(map_id) DO UPDATE SET name=excluded.name;