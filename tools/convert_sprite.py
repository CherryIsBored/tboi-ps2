from PIL import Image
import xml.etree.ElementTree as ET

NEAREST = Image.Resampling.NEAREST

def rgba_to_gs(image):
    values = []
    for r, g, b, a in image.convert("RGBA").getdata():
        alpha = (a * 128 + 127) // 255
        values.append(r | (g << 8) | (b << 16) | (alpha << 24))
    return values

def remove_white_matte(image, threshold=242):
    """Turn the white atlas canvas and its antialiased fringe into transparency."""
    image = image.convert("RGBA")
    px = image.load()
    for y in range(image.height):
        for x in range(image.width):
            r, g, b, a = px[x, y]
            if r >= threshold and g >= threshold and b >= threshold:
                px[x, y] = (r, g, b, 0)
            elif r >= 220 and g >= 220 and b >= 220:
                # Fade the near-white fringe instead of leaving a pale halo.
                strength = max(0, min(255, int((255 - min(r, g, b)) * 255 / 35)))
                px[x, y] = (r, g, b, min(a, strength))
    return image

def remove_black_matte(image, threshold=24):
    """Remove the black rectangle behind the supplied title logo."""
    image = image.convert("RGBA")
    px = image.load()
    for y in range(image.height):
        for x in range(image.width):
            r, g, b, a = px[x, y]
            if r <= threshold and g <= threshold and b <= threshold:
                px[x, y] = (r, g, b, 0)
    return image

def write_array(out, values, indent="    "):
    for i in range(0, len(values), 8):
        out.write(indent + ", ".join("0x%08Xu" % value for value in values[i:i + 8]) + ",\n")

def layer_frames(root, animation_name, layer_id):
    animation = root.find(".//Animation[@Name='%s']" % animation_name)
    if animation is None:
        raise SystemExit("Missing animation in player ANM2: " + animation_name)

    # The uploaded Isaac ANM2 stores the visible player layer as LayerId 0.
    # Some game dumps use the semantic body/head ids (1 and 4), so prefer the
    # requested id but safely fall back when this animation has one framed layer.
    layer = animation.find("./LayerAnimations/LayerAnimation[@LayerId='%s']" % layer_id)
    if layer is None:
        candidates = [
            item for item in animation.findall("./LayerAnimations/LayerAnimation")
            if item.findall("Frame")
        ]
        if len(candidates) != 1:
            raise SystemExit(
                "Missing layer %s in animation %s (found %d framed layers)" %
                (layer_id, animation_name, len(candidates))
            )
        layer = candidates[0]

    frames = [frame.attrib for frame in layer.findall("Frame")]
    if not frames:
        raise SystemExit("Empty layer %s in animation %s" % (layer_id, animation_name))
    return frames

def crop_anm_frame(atlas, attrs):
    x = int(float(attrs.get("XCrop", 0)))
    y = int(float(attrs.get("YCrop", 0)))
    w = int(float(attrs.get("Width", 32)))
    h = int(float(attrs.get("Height", 32)))
    crop = remove_white_matte(atlas.crop((x, y, x + w, y + h)))
    sx = float(attrs.get("XScale", 100))
    sy = float(attrs.get("YScale", 100))
    if sx < 0:
        crop = crop.transpose(Image.Transpose.FLIP_LEFT_RIGHT)
    if sy < 0:
        crop = crop.transpose(Image.Transpose.FLIP_TOP_BOTTOM)
    target_w = max(1, int(round(w * abs(sx) / 100.0)))
    target_h = max(1, int(round(h * abs(sy) / 100.0)))
    if crop.size != (target_w, target_h):
        crop = crop.resize((target_w, target_h), NEAREST)
    return crop

# Isaac: use the actual Walk and Head frame rectangles from the uploaded ANM2.
atlas = Image.open("assets/characters/character_001_isaac.png").convert("RGBA")
anm = ET.parse("assets/gfx/001.000_player.anm2").getroot()
directions = [
    ("WalkDown", "HeadDown"),
    ("WalkUp", "HeadUp"),
    ("WalkLeft", "HeadLeft"),
    ("WalkRight", "HeadRight"),
]
walk_frames = []
shoot_frames = []
for walk_name, head_name in directions:
    bodies = [crop_anm_frame(atlas, a) for a in layer_frames(anm, walk_name, 1)]
    heads = [crop_anm_frame(atlas, a) for a in layer_frames(anm, head_name, 4)]
    if not bodies or not heads:
        raise SystemExit("Empty Isaac animation: " + walk_name + " / " + head_name)
    normal_head = heads[0]
    shooting_head = heads[1] if len(heads) > 1 else heads[0]
    direction_walk = []
    for frame_index in range(10):
        body = bodies[frame_index % len(bodies)]
        frame = Image.new("RGBA", (32, 48), (0, 0, 0, 0))
        # ANM2 pivots: body top is y=-23 and head top is y=-33.
        # Shift both by +33 so the head starts at row 0 and the body at row 10.
        frame.alpha_composite(body, (0, 10))
        frame.alpha_composite(normal_head, (0, 0))
        direction_walk.append(rgba_to_gs(frame))
    shoot = Image.new("RGBA", (32, 48), (0, 0, 0, 0))
    shoot.alpha_composite(bodies[0], (0, 10))
    shoot.alpha_composite(shooting_head, (0, 0))
    walk_frames.append(direction_walk)
    shoot_frames.append(rgba_to_gs(shoot))

with open("sprite_data.h", "w") as out:
    out.write("#ifndef SPRITE_DATA_H\n#define SPRITE_DATA_H\n#include <stdint.h>\n")
    out.write("static const uint32_t isaac_pixels[4][10][1536] __attribute__((aligned(64))) = {\n")
    for direction in walk_frames:
        out.write("{\n")
        for frame in direction:
            out.write("{\n")
            write_array(out, frame)
            out.write("},\n")
        out.write("},\n")
    out.write("};\n")
    out.write("static const uint32_t isaac_shoot_pixels[4][1536] __attribute__((aligned(64))) = {\n")
    for frame in shoot_frames:
        out.write("{\n")
        write_array(out, frame)
        out.write("},\n")
    out.write("};\n#endif\n")

# Compose a complete Basement room from the real floor and wall artwork.
# The source wall sheet contains the top/left wall corner in its upper-left cell.
# Keep the floor at its native tile proportions and build four continuous wall runs.
basement = Image.open("assets/backdrop/01_basement.png").convert("RGBA")
floor = Image.open("assets/backdrop/01_lbasementfloor.png").convert("RGB").convert("RGBA")
room = Image.new("RGBA", (256, 256), (18, 14, 15, 255))

# Keep the complete original 2x2 floor tile pattern inside a 40px stone frame.
# The floor is deliberately opaque: this texture must never render as black.
floor_tile = floor.resize((176, 59), NEAREST)
for y in (40, 99, 158):
    room.alpha_composite(floor_tile, (40, y))

# Four continuous wall runs cut from the actual Basement wall atlas.
top_wall = basement.crop((10, 0, 238, 48)).resize((176, 40), NEAREST)
left_wall = basement.crop((0, 40, 52, 160)).resize((40, 176), NEAREST)
room.alpha_composite(top_wall, (40, 0))
room.alpha_composite(top_wall.transpose(Image.Transpose.FLIP_TOP_BOTTOM), (40, 216))
room.alpha_composite(left_wall, (0, 40))
room.alpha_composite(left_wall.transpose(Image.Transpose.FLIP_LEFT_RIGHT), (216, 40))

# Rebuild all four corners from the matching original wall corner tile.
corner = basement.crop((0, 0, 52, 52)).resize((40, 40), NEAREST)
room.alpha_composite(corner, (0, 0))
room.alpha_composite(corner.transpose(Image.Transpose.FLIP_LEFT_RIGHT), (216, 0))
room.alpha_composite(corner.transpose(Image.Transpose.FLIP_TOP_BOTTOM), (0, 216))
room.alpha_composite(corner.transpose(Image.Transpose.ROTATE_180), (216, 216))
with open("room_data.h", "w") as out:
    out.write("#ifndef ROOM_DATA_H\n#define ROOM_DATA_H\n#include <stdint.h>\n")
    out.write("static const uint32_t room_pixels[65536] __attribute__((aligned(64))) = {\n")
    write_array(out, rgba_to_gs(room))
    out.write("};\n#endif\n")


# Real animated fly frames from the uploaded Repentance sprite sheet.
flies = remove_white_matte(Image.open("assets/repentance/big_flies.png"), 238)
if flies.size != (128, 96):
    raise SystemExit("Unexpected big_flies.png dimensions: %s" % (flies.size,))
with open("enemy_data.h", "w") as out:
    out.write("#ifndef ENEMY_DATA_H\n#define ENEMY_DATA_H\n#include <stdint.h>\n")
    out.write("static const uint32_t enemy_pixels[4][1024] __attribute__((aligned(64))) = {\n")
    for x in (0, 32, 64, 96):
        out.write("{\n")
        write_array(out, rgba_to_gs(remove_white_matte(flies.crop((x, 0, x + 32, 32)), 238)))
        out.write("},\n")
    out.write("};\n#endif\n")

# Original title screen: paper background, Isaac logo and Press Start drawing.
title = Image.open("assets/main menu/titlemenu.png").convert("RGBA")
logo = Image.open("assets/main menu/logo.png").convert("RGBA")
menu_bg = Image.new("RGBA", (256, 256), (0, 0, 0, 255))
menu_bg.alpha_composite(title.crop((0, 0, 480, 272)).resize((256, 145), NEAREST), (0, 0))
# The title logo artwork occupies the upper part of logo.png; discard the
# unused black canvas and fit the full logo into a VRAM-safe 256x128 texture.
menu_logo = Image.new("RGBA", (256, 128), (0, 0, 0, 0))
logo_crop = remove_black_matte(logo.crop((98, 0, 373, 140)))
logo_crop.thumbnail((256, 124), NEAREST)
menu_logo.alpha_composite(logo_crop, ((256 - logo_crop.width) // 2, 2))
menu_prompt = Image.new("RGBA", (128, 128), (0, 0, 0, 0))
menu_prompt.alpha_composite(title.crop((0, 384, 156, 544)).resize((125, 128), NEAREST), (0, 0))
with open("menu_data.h", "w") as out:
    out.write("#ifndef MENU_DATA_H\n#define MENU_DATA_H\n#include <stdint.h>\n")
    for name, img, count in [
        ("menu_bg_pixels", menu_bg, 65536),
        ("menu_logo_pixels", menu_logo, 32768),
        ("menu_prompt_pixels", menu_prompt, 16384),
    ]:
        out.write("static const uint32_t %s[%d] __attribute__((aligned(64))) = {\n" % (name, count))
        write_array(out, rgba_to_gs(img))
        out.write("};\n")
    out.write("#endif\n")

# HUD hearts from the original UI atlas.
hearts = Image.open("ui_hearts.png").convert("RGBA")
heart = hearts.crop((0, 0, 16, 16))
with open("heart_data.h", "w") as out:
    out.write("#ifndef HEART_DATA_H\n#define HEART_DATA_H\n#include <stdint.h>\n")
    out.write("static const uint32_t heart_pixels[256] __attribute__((aligned(64))) = {\n")
    write_array(out, rgba_to_gs(heart))
    out.write("};\n#endif\n")

# Original pickup icons from the HUD atlas.
hud = Image.open("assets/ui/hudpickups.png").convert("RGBA")
# Runtime pickup order: heart, coin, key, bomb.
# HUD atlas positions: heart x=80, coin x=0, key x=64, bomb x=32.
pickup_cells = [(80, 0), (0, 0), (64, 0), (32, 0)]
with open("pickup_data.h", "w") as out:
    out.write("#ifndef PICKUP_DATA_H\n#define PICKUP_DATA_H\n#include <stdint.h>\n")
    out.write("static const uint32_t pickup_pixels[4][256] __attribute__((aligned(64))) = {\n")
    for x, y in pickup_cells:
        out.write("{\n")
        write_array(out, rgba_to_gs(hud.crop((x, y, x + 16, y + 16))))
        out.write("},\n")
    out.write("};\n#endif\n")
