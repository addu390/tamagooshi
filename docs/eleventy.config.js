export default function (eleventyConfig) {
  eleventyConfig.addGlobalData("permalink",
    "{{ page.filePathStem | replace_first: '/html/', '/' }}.html");

  for (const path of ["css", "js", "assets", "favicon.svg", "favicon.ico",
                      "apple-touch-icon.png", "CNAME"]) {
    eleventyConfig.addPassthroughCopy(path);
  }

  return {
    dir: { input: ".", output: "_site" },
  };
}
