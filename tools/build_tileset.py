"""
Combina los tiles individuales del pack Cute_Fantasy_Free en un solo tileset PNG.
Cada tile es 16x16. El atlas resultante tiene 16 columnas.

Tile index 0 = vacio (no se usa en la imagen).
Tile index 1 = primer tile del atlas (fila 0, col 0).

Incluye: terrenos (grass, path, water, farm, cliff, beach) + decoraciones del Outdoor_Decor_Free.
"""

from PIL import Image
import os

TILE = 16
COLS = 16  # columnas del atlas

base_tiles = os.path.join(os.path.dirname(__file__), '..', 'demo', 'assets', 'Cute_Fantasy_Free', 'Tiles')
base_decor = os.path.join(os.path.dirname(__file__), '..', 'demo', 'assets', 'Cute_Fantasy_Free', 'Outdoor decoration')

def extract_tiles(path):
    """Extrae tiles de 16x16 de una imagen, row-major."""
    img = Image.open(path).convert('RGBA')
    w, h = img.size
    cols = w // TILE
    rows = h // TILE
    tiles = []
    for r in range(rows):
        for c in range(cols):
            tile = img.crop((c * TILE, r * TILE, (c+1) * TILE, (r+1) * TILE))
            tiles.append(tile)
    return tiles

def is_empty_tile(tile):
    """Verifica si un tile es completamente transparente."""
    data = tile.getdata()
    return all(px[3] == 0 for px in data)

# Recolectar todos los tiles en orden
all_tiles = []

# ── TERRENOS ──

# 1. Singles (16x16)
print("Loading singles...")
for name in ['Grass_Middle.png', 'Path_Middle.png', 'Water_Middle.png']:
    all_tiles.extend(extract_tiles(os.path.join(base_tiles, name)))
    print(f"  {name}: 1 tile (total: {len(all_tiles)})")

# 2. FarmLand (48x48 = 9 tiles)
print("Loading FarmLand_Tile...")
all_tiles.extend(extract_tiles(os.path.join(base_tiles, 'FarmLand_Tile.png')))
print(f"  FarmLand_Tile: 9 tiles (total: {len(all_tiles)})")

# 3. Path_Tile (48x96 = 18 tiles)
print("Loading Path_Tile...")
all_tiles.extend(extract_tiles(os.path.join(base_tiles, 'Path_Tile.png')))
print(f"  Path_Tile: 18 tiles (total: {len(all_tiles)})")

# 4. Water_Tile (48x96 = 18 tiles)
print("Loading Water_Tile...")
all_tiles.extend(extract_tiles(os.path.join(base_tiles, 'Water_Tile.png')))
print(f"  Water_Tile: 18 tiles (total: {len(all_tiles)})")

# 5. Cliff_Tile (48x96 = 18 tiles)
print("Loading Cliff_Tile...")
all_tiles.extend(extract_tiles(os.path.join(base_tiles, 'Cliff_Tile.png')))
print(f"  Cliff_Tile: 18 tiles (total: {len(all_tiles)})")

# 6. Beach_Tile (80x48 = 15 tiles)
print("Loading Beach_Tile...")
all_tiles.extend(extract_tiles(os.path.join(base_tiles, 'Beach_Tile.png')))
print(f"  Beach_Tile: 15 tiles (total: {len(all_tiles)})")

# ── DECORACIONES (16x16 del Outdoor_Decor_Free.png) ──
print("\nLoading Outdoor_Decor_Free (16x16 tiles)...")
decor_path = os.path.join(base_decor, 'Outdoor_Decor_Free.png')
decor_tiles_raw = extract_tiles(decor_path)
# Filtrar tiles vacios (transparentes)
decor_tiles = [t for t in decor_tiles_raw if not is_empty_tile(t)]
all_tiles.extend(decor_tiles)
print(f"  Outdoor_Decor_Free: {len(decor_tiles)} non-empty tiles (total: {len(all_tiles)})")

# ── FENCES (16x16 del Fences.png 64x64 = 4x4 grid) ──
print("Loading Fences...")
fence_tiles_raw = extract_tiles(os.path.join(base_decor, 'Fences.png'))
fence_tiles = [t for t in fence_tiles_raw if not is_empty_tile(t)]
all_tiles.extend(fence_tiles)
print(f"  Fences: {len(fence_tiles)} non-empty tiles (total: {len(all_tiles)})")

# ── CHEST (16x16) ──
print("Loading Chest...")
all_tiles.extend(extract_tiles(os.path.join(base_decor, 'Chest.png')))
print(f"  Chest: 1 tile (total: {len(all_tiles)})")

# Crear atlas
total = len(all_tiles)
rows_needed = (total + COLS - 1) // COLS
atlas_w = COLS * TILE
atlas_h = rows_needed * TILE

print(f"\nAtlas: {COLS} cols x {rows_needed} rows = {atlas_w}x{atlas_h} px")
print(f"Total tiles: {total}")

atlas = Image.new('RGBA', (atlas_w, atlas_h), (0, 0, 0, 0))

for i, tile in enumerate(all_tiles):
    col = i % COLS
    row = i // COLS
    atlas.paste(tile, (col * TILE, row * TILE))

out_path = os.path.join(os.path.dirname(__file__), '..', 'demo', 'assets', 'tileset.png')
atlas.save(out_path)
print(f"\nSaved: {out_path}")

# Imprimir mapa de tiles para referencia
print("\n=== TILE INDEX MAP ===")
print("(index 0 = vacio, index 1 = primer tile)")
idx = 1
names = [
    ("Grass",     1),
    ("PathMid",   1),
    ("WaterMid",  1),
    ("FarmLand",  9),
    ("Path",     18),
    ("Water",    18),
    ("Cliff",    18),
    ("Beach",    15),
    ("Decor",    len(decor_tiles)),
    ("Fences",   len(fence_tiles)),
    ("Chest",     1),
]
for name, count in names:
    print(f"  {name}: tiles {idx}..{idx + count - 1}")
    idx += count
