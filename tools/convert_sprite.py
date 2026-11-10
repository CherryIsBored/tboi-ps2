from PIL import Image

source = "assets/character_001_isaac.png"
image = Image.open(source).convert("RGBA")
if image.size != (512, 512):
    raise SystemExit("Expected 512x512 Isaac atlas, got %s" % (image.size,))

# WalkDown's body frame is the 32x32 tile at (0, 32).
# Isaac's face is the matching first head tile at (0, 0).
body = image.crop((0, 32, 32, 64))
head = image.crop((0, 0, 32, 32))

# Composite the head over the body, preserving the transparent pixels.
frame = Image.new("RGBA", (32, 32), (0, 0, 0, 0))
frame.alpha_composite(body, (0, 0))
frame.alpha_composite(head, (0, 0))

pixels = []
for r, g, b, a in frame.getdata():
    pixels.append(r | (g << 8) | (b << 16) | (a << 24))

with open("sprite_data.h", "w") as out:
    out.write("#ifndef SPRITE_DATA_H\n#define SPRITE_DATA_H\n")
    out.write("#include <stdint.h>\n")
    out.write("static const uint32_t isaac_pixels[1024] __attribute__((aligned(64))) = {\n")
    for i in range(0, len(pixels), 8):
        out.write("    " + ", ".join("0x%08Xu" % p for p in pixels[i:i+8]) + ",\n")
    out.write("};\n#endif\n")
