const FILES = {
  pavement: "ground/pavement.png",
  slab: "ground/slab.png",
  grate: "ground/grate.png",
  rock: "ground/rock.png",
  crate: "crate/box.png",
  crate_planks: "crate/planks.png",
  crate_boarded: "crate/boarded.png",
  crate_player: "crate/player.png",
  door_lit: "door/lit.png",
  door_shut: "door/shut.png",
  door_metal: "door/metal.png",
  window: "window/dark.png",
  window_lit: "window/lit.png",
  wall_brick: "wall/brick.png",
  wall_brick_l: "wall/brick_l.png",
  wall_brick_r: "wall/brick_r.png",
  wall_sand: "wall/sand.png",
  wall_sand_l: "wall/sand_l.png",
  wall_sand_r: "wall/sand_r.png",
  wall_stone: "wall/stone.png",
  wall_timber: "wall/timber.png",
  timber_horiz: "wall/timber_horiz.png",
  frame_clay: "wall/clay.png",
  roof_red: "roof/red.png",
  roof_grey: "roof/grey.png",
  thatch: "roof/thatch.png",
  thatch_edge: "roof/thatch_edge.png",
};

export async function loadTextures() {
  const entries = await Promise.all(Object.entries(FILES).map(async ([name, file]) => {
    const img = new Image();
    img.src = `assets/${file}`;
    await img.decode();
    return [name, img];
  }));
  return Object.fromEntries(entries);
}
