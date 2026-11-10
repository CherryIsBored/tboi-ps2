from PIL import Image

source = "assets/character_001_isaac.png"
image = Image.open(source).convert("RGBA")
if image.size != (512, 512):
    raise SystemExit("Expected 512x512 Isaac atlas, got %s" % (image.size,))

# ANM2 WalkDown/WalkUp use row 32; WalkLeft/WalkRight use row 64.
# Head animations: down=(0,0), up=(128,0), side=(64,0).
directions = [
    {"body_y": 32, "head_x": 0, "mirror": False},   # down
    {"body_y": 32, "head_x": 128, "mirror": False}, # up
    {"body_y": 64, "head_x": 64, "mirror": True},   # left
    {"body_y": 64, "head_x": 64, "mirror": False},  # right
]

frames = []
for direction in directions:
    direction_frames = []
    for frame_index in range(10):
        body_x = (frame_index * 32) if frame_index < 8 else ((frame_index - 8) * 32 + 192)
        body = image.crop((body_x, direction["body_y"], body_x + 32, direction["body_y"] + 32))
        head_x = direction["head_x"] + ((frame_index // 5) * 32)
        head = image.crop((head_x, 0, head_x + 32, 32))
        if direction["mirror"]:
            body = body.transpose(Image.Transpose.FLIP_LEFT_RIGHT)
            head = head.transpose(Image.Transpose.FLIP_LEFT_RIGHT)

        frame = Image.new("RGBA", (32, 48), (0, 0, 0, 0))
        frame.alpha_composite(body, (0, 8))
        # Keep the head close to the body; raised two pixels from the original crop.
        frame.alpha_composite(head, (0, -2))

        pixels = []
        for r, g, b, a in frame.getdata():
            gs_alpha = (a * 128 + 127) // 255
            pixels.append(r | (g << 8) | (b << 16) | (gs_alpha << 24))
        direction_frames.append(pixels)
    frames.append(direction_frames)

with open("sprite_data.h", "w") as out:
    out.write("#ifndef SPRITE_DATA_H\\n#define SPRITE_DATA_H\\n")
    out.write("#include <stdint.h>\\n")
    out.write("static const uint32_t isaac_pixels[4][10][1536] __attribute__((aligned(64))) = {\\n")
    for direction_frames in frames:
        out.write("{\\n")
        for pixels in direction_frames:
            out.write("{\\n")
            for i in range(0, len(pixels), 8):
                out.write("    " + ", ".join("0x%08Xu" % p for p in pixels[i:i+8]) + ",\\n")
            out.write("},\\n")
        out.write("},\\n")
    out.write("};\\n#endif\\n")
