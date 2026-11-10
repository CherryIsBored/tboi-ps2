from PIL import Image
import xml.etree.ElementTree as ET

NEAREST = Image.Resampling.NEAREST

def rgba_to_gs(image):
    values = []
    for r, g, b, a in image.convert("RGBA").getdata():
        alpha = (a * 128 + 127) // 255
        values.append(r | (g << 8) | (b << 16) | (alpha << 24))
    return values

def write_array(out, values, indent="    "):
    for i in range(0, len(values), 8):
        out.write(indent + ", ".join("0x%08Xu" % value for value in values[i:i + 8]) + ",\n")

def layer_frames(root, animation_name, layer_id):
    animation = root.find(".//Animation[@Name='%s']" % animation_name)
    if animation is None:
        raise SystemExit("Missing animation in player ANM2: " + animation_name)
    layer = animation.find("./LayerAnimations/LayerAnimation[@LayerId='%s']" % layer_id)
    if layer is None:
        raise SystemExit("Missing layer %s in animation %s" % (layer_id, animation_name))
    return [frame.attrib for frame in layer.findall("Frame")]

def crop_anm_frame(atlas, attrs):
    x = int(float(attrs.get("XCrop", 0)))
    y = int(float(attrs.get("YCrop", 0)))
    w = int(float(attrs.get("Width", 32)))
    h = int(float(attrs.get("Height", 32)))
    crop = atlas.crop((x, y, x + w, y + h)).resize((32, 32), NEAREST)
    if float(attrs.get("XScale", 100)) < 0:
        crop = crop.transpose(Image.Transpose.FLIP_LEFT_RIGHT)
    if float(attrs.get("YScale", 100)) < 0:
        crop = crop.transpose(Image.Transpose.FLIP_TOP_BOTTOM)
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
        frame.alpha_composite(body, (0, 8))
        frame.alpha_composite(normal_head, (0, -2))
        direction_walk.append(rgba_to_gs(frame))
    shoot = Image.new("RGBA", (32, 48), (0, 0, 0, 0))
    shoot.alpha_composite(bodies[0], (0, 8))
    shoot.alpha_composite(shooting_head, (0, -2))
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

# Original Basement room: tiled floor plus wall strips cut from the actual room atlas.
basement = Image.open("assets/backdrop/01_basement.png").convert("RGBA")
floor = Image.open("assets/backdrop/01_lbasementfloor.png").convert("RGBA")
room = Image.new("RGBA", (256, 256), (0, 0, 0, 255))
floor_tile = floor.resize((256, 91), NEAREST)
room.alpha_composite(floor_tile, (0, 32))
room.alpha_composite(floor_tile, (0, 123))
room.alpha_composite(floor_tile.crop((0, 0, 256, 10)), (0, 214))
wall_strip = basement.crop((0, 0, 240, 48)).resize((256, 32), NEAREST)
room.alpha_composite(wall_strip, (0, 0))
room.alpha_composite(wall_strip.transpose(Image.Transpose.FLIP_TOP_BOTTOM), (0, 224))
side_strip = wall_strip.transpose(Image.Transpose.ROTATE_90).resize((32, 256), NEAREST)
room.alpha_composite(side_strip, (0, 0))
room.alpha_composite(side_strip.transpose(Image.Transpose.FLIP_LEFT_RIGHT), (224, 0))
with open("room_data.h", "w") as out:
    out.write("#ifndef ROOM_DATA_H\n#define ROOM_DATA_H\n#include <stdint.h>\n")
    out.write("static const uint32_t room_pixels[65536] __attribute__((aligned(64))) = {\n")
    write_array(out, rgba_to_gs(room))
    out.write("};\n#endif\n")


# Real animated fly frames from the uploaded Repentance sprite sheet.
flies = Image.open("assets/repentance/big_flies.png").convert("RGBA")
if flies.size != (128, 96):
    raise SystemExit("Unexpected big_flies.png dimensions: %s" % (flies.size,))
with open("enemy_data.h", "w") as out:
    out.write("#ifndef ENEMY_DATA_H\n#define ENEMY_DATA_H\n#include <stdint.h>\n")
    out.write("static const uint32_t enemy_pixels[4][1024] __attribute__((aligned(64))) = {\n")
    for x in (0, 32, 64, 96):
        out.write("{\n")
        write_array(out, rgba_to_gs(flies.crop((x, 0, x + 32, 32))))
        out.write("},\n")
    out.write("};\n#endif\n")

# Original title screen: paper background, Isaac logo and Press Start drawing.
title = Image.open("assets/main menu/titlemenu.png").convert("RGBA")
logo = Image.open("assets/main menu/logo.png").convert("RGBA")
menu_bg = Image.new("RGBA", (256, 256), (0, 0, 0, 0))
menu_bg.alpha_composite(title.crop((0, 0, 480, 272)).resize((256, 145), NEAREST), (0, 0))
menu_logo = Image.new("RGBA", (256, 128), (0, 0, 0, 0))
menu_logo.alpha_composite(logo.crop((0, 0, 480, 160)).resize((256, 85), NEAREST), (0, 0))
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
pickup_cells = [(0, 0), (32, 0), (64, 0), (80, 0)]
with open("pickup_data.h", "w") as out:
    out.write("#ifndef PICKUP_DATA_H\n#define PICKUP_DATA_H\n#include <stdint.h>\n")
    out.write("static const uint32_t pickup_pixels[4][256] __attribute__((aligned(64))) = {\n")
    for x, y in pickup_cells:
        out.write("{\n")
        write_array(out, rgba_to_gs(hud.crop((x, y, x + 16, y + 16))))
        out.write("},\n")
    out.write("};\n#endif\n")
