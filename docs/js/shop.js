(function () {
  const BOARDS = [
    { id: "m5sticks3", name: "M5StickS3", price: 21.5, checkout: "",
      url: "https://shop.m5stack.com/products/m5sticks3-esp32s3-mini-iot-dev-kit" },
    { id: "m5stickc-plus-se", name: "M5StickC Plus SE", price: 19, checkout: "",
      url: "https://shop.m5stack.com/products/m5stickc-plus-se-mini-iot-dev-kit-esp32-pico" },
  ];
  const EXTRAS = 10 + 10 + 10;
  const DIY_SHIPPING = 10;
  const fmt = (v) => "$" + (Number.isInteger(v) ? v : v.toFixed(2));
  const board = (select) => BOARDS.find((b) => b.id === select.value);

  document.querySelectorAll("#built-board, #diy-board").forEach((select) => {
    BOARDS.forEach((b) => {
      const opt = document.createElement("option");
      opt.value = b.id;
      opt.textContent = b.name;
      select.appendChild(opt);
    });
  });

  const built = document.getElementById("built-board");
  const applyBuilt = () => {
    const b = board(built);
    document.getElementById("built-hw").textContent = fmt(b.price);
    document.getElementById("built-total").textContent = fmt(b.price + EXTRAS);
    document.getElementById("built-price").textContent = fmt(b.price + EXTRAS);
    const checkout = document.getElementById("built-checkout");
    if (b.checkout) {
      checkout.href = b.checkout;
      checkout.removeAttribute("aria-disabled");
    }
  };
  built.addEventListener("change", applyBuilt);
  applyBuilt();

  const diy = document.getElementById("diy-board");
  const applyDiy = () => {
    const b = board(diy);
    document.getElementById("diy-hw").textContent = fmt(b.price);
    document.getElementById("diy-total").textContent = "~" + fmt(b.price + DIY_SHIPPING);
    document.getElementById("diy-price").textContent = "~" + fmt(b.price + DIY_SHIPPING);
    document.getElementById("diy-buy").href = b.url;
  };
  diy.addEventListener("change", applyDiy);
  applyDiy();
})();
