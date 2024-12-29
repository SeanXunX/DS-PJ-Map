import mapnik
import os
import math

# 瓦片输出目录
tile_output_dir = './tiles'
os.makedirs(tile_output_dir, exist_ok=True)

# 地图对象
mapfile = 'file.xml'  # 样式文件路径
tile_size = 256
m = mapnik.Map(tile_size, tile_size)
mapnik.load_map(m, mapfile)

# 上海经纬度范围
shanghai_bounds = {
    "min_lat": 30.6592375,
    "max_lat": 31.8756055,
    "min_lon": 120.845924,
    "max_lon": 122.2430515,
}

# 经纬度 -> 瓦片索引
def deg2num(lat_deg, lon_deg, zoom):
    lat_rad = math.radians(lat_deg)
    n = 1 << zoom
    xtile = int((lon_deg + 180.0) / 360.0 * n)
    ytile = int((1.0 - math.asinh(math.tan(lat_rad)) / math.pi) / 2.0 * n)
    return xtile, ytile

# 瓦片索引 -> 经纬度
def num2deg(xtile, ytile, zoom):
    n = 1 << zoom
    lon_deg = xtile / n * 360.0 - 180.0
    lat_rad = math.atan(math.sinh(math.pi * (1 - 2 * ytile / n)))
    lat_deg = math.degrees(lat_rad)
    return lon_deg, lat_deg

# 计算瓦片索引范围
def calculate_tile_range(bounds, zoom):
    min_x, max_y = deg2num(bounds["min_lat"], bounds["min_lon"], zoom)
    max_x, min_y = deg2num(bounds["max_lat"], bounds["max_lon"], zoom)
    return min_x, max_x, min_y, max_y

# 渲染瓦片函数
def render_tile(z, x, y):
    # 瓦片的边界经纬度
    lon1, lat1 = num2deg(x, y, z)
    lon2, lat2 = num2deg(x + 1, y + 1, z)
    tile_bbox = mapnik.Box2d(lon1, lat1, lon2, lat2)
    
    # print(f"lon1, lat1 = {lon1, lat1}\t lon2, lat2 = {lon2, lat2}")

    # 调整地图范围并渲染
    m.aspect_fix_mode = mapnik.aspect_fix_mode.RESPECT  # 防止强制调整比例
    m.zoom_to_box(tile_bbox)
    
    # print(f"Tile BBox: {tile_bbox}")
    # print(f"Mapnik Envelope: {m.envelope()}")

    img = mapnik.Image(tile_size, tile_size)
    mapnik.render(m, img)

    # 保存瓦片
    tile_dir = os.path.join(tile_output_dir, str(z), str(x))
    os.makedirs(tile_dir, exist_ok=True)
    tile_path = os.path.join(tile_dir, f"{y}.png")
    img.save(tile_path, 'png')

# 渲染瓦片的循环
min_zoom = 0  # 最小缩放级别
max_zoom = 18  # 最大缩放级别

for z in range(min_zoom, max_zoom + 1):
    min_x, max_x, min_y, max_y = calculate_tile_range(shanghai_bounds, z)
    print(f"Zoom level {z}: x range [{min_x}, {max_x}], y range [{min_y}, {max_y}]")
    
    for x in range(min_x, max_x + 1):
        for y in range(min_y, max_y + 1):
            print(f"Rendering tile z={z}, x={x}, y={y}")
            render_tile(z, x, y)
