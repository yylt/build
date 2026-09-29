#!/usr/bin/env node
/**
 * Strip the "GitHub / Follow us (关注我们) / Sign up" promotional cluster from the
 * `excalidraw-app` build at build time.
 *
 * Removals are idempotent: running the script more than once is a no-op once the
 * blocks are already gone, so it is safe to wire this into `build:app` / `build`.
 *
 * Affected files (app only — the published `@excalidraw/excalidraw` package is
 * intentionally left untouched):
 *   - excalidraw-app/App.tsx                  (command-palette GitHub / Follow us / Sign up)
 *   - excalidraw-app/components/AppMainMenu.tsx   (Socials group + Sign up item)
 *   - excalidraw-app/components/AppWelcomeScreen.tsx (Sign up link)
 */

const fs = require("fs");
const path = require("path");

const repoRoot = path.resolve(__dirname, "..");

const FILES = {
  app: path.join(repoRoot, "excalidraw-app", "App.tsx"),
  mainMenu: path.join(repoRoot, "excalidraw-app", "components", "AppMainMenu.tsx"),
  welcome: path.join(
    repoRoot,
    "excalidraw-app",
    "components",
    "AppWelcomeScreen.tsx",
  ),
};

let changed = 0;

function editFile(filePath, transforms) {
  if (!fs.existsSync(filePath)) {
    console.warn(`[strip-promo] skip (missing): ${path.relative(repoRoot, filePath)}`);
    return;
  }
  let content = fs.readFileSync(filePath, "utf8");
  const rel = path.relative(repoRoot, filePath);

  for (const { name, apply } of transforms) {
    const before = content;
    content = apply(content);
    if (content !== before) {
      changed++;
      console.log(`[strip-promo] removed: ${rel} -> ${name}`);
    }
  }

  fs.writeFileSync(filePath, content);
}

/**
 * Remove a JS/TS object literal whose first property matches `labelRegex`.
 * Works whether the opening `{` is on its own line or trailing a declaration.
 * Also consumes a single trailing `,` so array element separators stay valid.
 */
function removeJsObject(content, labelRegex) {
  const idx = content.search(labelRegex);
  if (idx === -1) {
    return content;
  }

  // Walk backwards from the label to find the matching opening brace.
  let depth = 0;
  let start = -1;
  for (let i = idx; i >= 0; i--) {
    const c = content[i];
    if (c === "}") depth++;
    else if (c === "{") {
      depth--;
      if (depth < 0) {
        start = i;
        break;
      }
    }
  }
  if (start === -1) return content;

  // Walk forwards to find the matching closing brace.
  depth = 0;
  let end = -1;
  for (let i = start; i < content.length; i++) {
    const c = content[i];
    if (c === "{") depth++;
    else if (c === "}") {
      depth--;
      if (depth === 0) {
        end = i;
        break;
      }
    }
  }
  if (end === -1) return content;

  // Consume a single trailing separator comma, if present.
  let tail = end + 1;
  while (tail < content.length && (content[tail] === " " || content[tail] === "\t")) {
    tail++;
  }
  if (content[tail] === ",") tail++;

  return content.slice(0, start) + content.slice(tail);
}

// ---------------------------------------------------------------------------
// excalidraw-app/App.tsx
// ---------------------------------------------------------------------------
editFile(FILES.app, [
  {
    name: "command-palette GitHub command",
    apply: (c) => removeJsObject(c, /label:\s*"GitHub"/),
  },
  {
    name: "command-palette Follow-us (关注我们) command",
    apply: (c) =>
      removeJsObject(c, /label:\s*t\(\s*["']labels\.followUs["']\s*\)/),
  },
  {
    name: "command-palette Excalidraw+ (links) const",
    // The "Excalidraw+" command is the only remaining `DEFAULT_CATEGORIES.links`
    // item defined as a const. Remove the const so the whole `links` category is
    // gone from the palette. (Its reference is removed by the next transform.)
    apply: (c) =>
      c.replace(
        /\n[ \t]*const ExcalidrawPlusCommand = \{[\s\S]*?\n[ \t]*\};/,
        "",
      ),
  },
  {
    name: "command-palette Discord (links) command",
    apply: (c) => removeJsObject(c, /label:\s*t\(\s*["']labels\.discordChat["']\s*\)/),
  },
  {
    name: "command-palette YouTube (links) command",
    apply: (c) => removeJsObject(c, /label:\s*"YouTube"/),
  },
  {
    name: "command-palette Sign-up (ExcalidrawPlusAppCommand) const",
    apply: (c) =>
      c.replace(
        /\n[ \t]*const ExcalidrawPlusAppCommand = \{[\s\S]*?\n[ \t]*\};/,
        "",
      ),
  },
  {
    name: "command-palette signed-user spread (references removed links consts)",
    // The whole `...(isExcalidrawPlusSignedUser ? [ ...ExcalidrawPlusAppCommand ] : [ExcalidrawPlusCommand, ExcalidrawPlusAppCommand])`
    // block references `links`-category consts that are being removed. Drop the
    // entire spread so no dangling reference remains.
    apply: (c) =>
      c.replace(
        /\n[ \t]*\.\.\.\(isExcalidrawPlusSignedUser[\s\S]*?ExcalidrawPlusAppCommand\]\),/,
        "\n",
      ),
  },
  {
    name: "unused GithubIcon import",
    apply: (c) => c.replace(/\n[ \t]*GithubIcon,/, ""),
  },
  {
    name: "unused XBrandIcon import",
    apply: (c) => c.replace(/\n[ \t]*XBrandIcon,/, ""),
  },
  {
    name: "unused ExcalLogo import (links items removed)",
    apply: (c) => c.replace(/\n[ \t]*ExcalLogo,/, ""),
  },
  {
    name: "unused DiscordIcon import (links items removed)",
    apply: (c) => c.replace(/\n[ \t]*DiscordIcon,/, ""),
  },
  {
    name: "unused youtubeIcon import (links items removed)",
    apply: (c) => c.replace(/\n[ \t]*youtubeIcon,/, ""),
  },
]);

// ---------------------------------------------------------------------------
// excalidraw-app/components/AppMainMenu.tsx
// ---------------------------------------------------------------------------
editFile(FILES.mainMenu, [
  {
    name: "Socials (GitHub / Follow us / Discord) menu group",
    apply: (c) => c.replace(/\n[ \t]*<MainMenu\.DefaultItems\.Socials \/>/, ""),
  },
  {
    name: "Sign-up ItemLink block",
    apply: (c) =>
      c.replace(
        /\n[ \t]*<MainMenu\.ItemLink\n[ \t]*icon=\{loginIcon\}[\s\S]*?[ \t]*<\/MainMenu\.ItemLink>\n/,
        "\n",
      ),
  },
  {
    name: "unused loginIcon import",
    apply: (c) => c.replace(/\n[ \t]*loginIcon,/, ""),
  },
  {
    name: "unused useI18n import (only `t` was used by the removed block)",
    apply: (c) =>
      c.replace(/\nimport \{ useI18n \} from "@excalidraw\/excalidraw\/i18n";/, ""),
  },
  {
    name: "unused isExcalidrawPlusSignedUser import",
    apply: (c) =>
      c.replace(
        /\nimport \{ isExcalidrawPlusSignedUser \} from "\.\.\/app_constants";/,
        "",
      ),
  },
  {
    name: "unused `const { t } = useI18n()`",
    apply: (c) => c.replace(/\n[ \t]*const \{ t \} = useI18n\(\);/, ""),
  },
]);

// ---------------------------------------------------------------------------
// excalidraw-app/components/AppWelcomeScreen.tsx
// ---------------------------------------------------------------------------
editFile(FILES.welcome, [
  {
    name: "Sign-up MenuItemLink block",
    // Brace-balanced removal: start at the JSX expression container `{!isExcalidrawPlusSignedUser && (`
    // and consume through its matching closing `}`. (A naive `.*?` match would stop early at the
    // inner `{t("labels.signUp")}`'s `)}` and orphan the element's closing tags.)
    apply: (c) => removeJsObject(c, /\{!isExcalidrawPlusSignedUser/),
  },
  {
    name: "unused loginIcon import",
    apply: (c) =>
      c.replace(
        /import \{ loginIcon \} from "@excalidraw\/excalidraw\/components\/icons";\n/,
        "",
      ),
  },
]);

console.log(
  changed > 0
    ? `[strip-promo] done — ${changed} block(s) removed.`
    : "[strip-promo] nothing to remove (already stripped).",
);
