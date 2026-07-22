export default function (eleventyConfig) {
  eleventyConfig.addGlobalData("permalink",
    "{{ page.filePathStem | replace_first: '/docs/', '/' | replace_first: '/mixer/mixer', '/mixer' }}.html");

  for (const path of ["common", "docs/js", "docs/assets", "mixer/js", "mixer/css",
                      "mixer/assets", "games/**/*.{js,css}", "games/**/assets/**",
                      "favicon.svg", "favicon.ico", "apple-touch-icon.png", "CNAME"]) {
    eleventyConfig.addPassthroughCopy(path);
  }
  eleventyConfig.ignores.add("docs/tools/**");

  return {
    dir: { input: ".", output: "_site" },
  };
}
