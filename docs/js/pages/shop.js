import { initChrome } from "../components/chrome.js";
import { initKeyHints } from "../components/keyhints.js";
import { initAccentPicker } from "../components/accents.js";
import { singleSelect } from "../components/dropdown.js";

initChrome();
initKeyHints();
initAccentPicker();

const BOARDS = [
  { id: "m5sticks3", name: "M5StickS3", price: 21.5, checkout: "",
    url: "https://shop.m5stack.com/products/m5sticks3-esp32s3-mini-iot-dev-kit" },
  { id: "m5stickc-plus-se", name: "M5StickC Plus SE", price: 19, checkout: "",
    url: "https://shop.m5stack.com/products/m5stickc-plus-se-mini-iot-dev-kit-esp32-pico" },
];
const EXTRAS = 10 + 10 + 10;
const DIY_SHIPPING = 10;
const fmt = (v) => "$" + (Number.isInteger(v) ? v : v.toFixed(2));

function boardPicker(mountId, apply) {
  let board = BOARDS[0];

  const picker = singleSelect(() => BOARDS.map((b) => [b.id, b.name]), () => board.id,
                              (id) => { board = BOARDS.find((b) => b.id === id); apply(board); },
                              { variant: "slim float", ariaLabel: "Board" });

  document.getElementById(mountId).appendChild(picker);
  apply(board);
}

boardPicker("built-board", (b) => {
  document.getElementById("built-hw").textContent = fmt(b.price);
  document.getElementById("built-total").textContent = fmt(b.price + EXTRAS);
  document.getElementById("built-price").textContent = fmt(b.price + EXTRAS);

  const checkout = document.getElementById("built-checkout");
  if (b.checkout) {
    checkout.href = b.checkout;
    checkout.removeAttribute("aria-disabled");
  }
});

boardPicker("diy-board", (b) => {
  document.getElementById("diy-hw").textContent = fmt(b.price);
  document.getElementById("diy-total").textContent = "~" + fmt(b.price + DIY_SHIPPING);
  document.getElementById("diy-price").textContent = "~" + fmt(b.price + DIY_SHIPPING);
  document.getElementById("diy-buy").href = b.url;
});
