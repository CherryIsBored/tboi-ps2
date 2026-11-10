from PIL import Image

source = "assets/character_001_isaac.png"
image = Image.open(source).convert("RGBA")
if image.size != (512, 512):
    raise SystemExit("Expected 512x512 Isaac atlas, got %s" % (image.size,))

# WalkDown atlas: body cell at (0, 32), head cell at (0, 0).
# Keep the two layers overlapping at the neck instead of stacking them apart.
body = image.crop((0, 32, 32, 64))
head = image.crop((0, 0, 32, 32))

frame = Image.new("RGBA", (32, 48), (0, 0, 0, 0))
frame.alpha_composite(body, (0, 8))
frame.alpha_composite(head, (0, 0))

pixels = []
for r, g, b, a in frame.getdata():
    # PS2 GS texture alpha uses 0x80 as fully opaque.
    gs_alpha = (a * 128 + 127) // 255
    pixels.append(r | (g << 8) | (b << 16) | (gs_alpha << 24))

with open("sprite_data.h", "w") as out:
    out.write("#ifndef SPRITE_DATA_H\n#define SPRITE_DATA_H\n")
    out.write("#include <stdint.h>\n")
    out.write("static const uint32_t isaac_pixels[1536] __attribute__((aligned(64))) = {\n")
    for i in range(0, len(pixels), 8):
        out.write("    " + ", ".join("0x%08Xu" % p for p in pixels[i:i+8]) + ",\n")
    out.write("};\n#endif\n")
