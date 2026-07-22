import { brandLockup } from "../../common/js/components/brand.js";

const nav = document.createElement("nav");
nav.className = "game-brand";
nav.innerHTML = brandLockup();
document.body.append(nav);
