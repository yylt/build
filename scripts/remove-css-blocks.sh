#!/usr/bin/env bash
#
# remove-css-blocks.sh — 从 CSS Modules 源文件中按选择器名删除顶层规则块。
#
# 鲁棒点：
#   * 按花括号深度逐行解析，只在 depth==0 匹配顶层选择器后才开始跳过，
#     直到回到 depth==0 的闭合 '}'，因此不会误删嵌套在 @media / & 里的规则。
#   * 匹配的是 `.name` 开头后跟空格/逗号/'{' 的选择器，
#     所以 .info 会被删，而 .info-link / .info-content 不受影响。
#   * 删除块后把 3 个及以上连续空行收紧为 1 个空行，保持文件整洁。
#   * 失败（文件不存在/无写权限）时以非 0 退出，且不破坏原文件。
#
# 用法:
#   remove-css-blocks.sh <css-file> [selector ...]
#   不传选择器时默认删除 info 和 footer。
#
set -euo pipefail

file="${1:?用法: $0 <css-file> [selectors...]}"
shift
selectors=("$@")
if [ "${#selectors[@]}" -eq 0 ]; then
  selectors=(info footer)
fi

[ -f "$file" ] || { echo "错误: 找不到文件 $file" >&2; exit 1; }

# 把选择器列表拼成正则 alternation: info|footer
alt=""
for s in "${selectors[@]}"; do
  alt="${alt:+$alt|}$s"
done

tmp="$(mktemp)"
trap 'rm -f "$tmp"' EXIT

skip=0      # >0 表示当前处于被删除的块内（值为该块的起始深度）
depth=0
removed=0

while IFS= read -r line || [ -n "$line" ]; do
  opens=$(grep -o '{' <<<"$line" | wc -l || true)
  closes=$(grep -o '}' <<<"$line" | wc -l || true)

  if [ "$skip" -eq 0 ]; then
    # 仅在顶层 (depth==0) 且行首是该选择器时，删除整块
    if [ "$depth" -eq 0 ] && [[ "$line" =~ ^[[:space:]]*\.($alt)[[:space:]]*[,{] ]]; then
      skip=1
      depth=$((depth + opens - closes))
      removed=$((removed + 1))
      continue
    fi
    printf '%s\n' "$line"
    depth=$((depth + opens - closes))
  else
    depth=$((depth + opens - closes))
    if [ "$depth" -le 0 ]; then
      skip=0   # 块已闭合，不输出闭合花括号那一行
    fi
  fi
done < "$file" > "$tmp"

# 收紧多余空行：3 个及以上连续空行 → 1 个
awk 'BEGIN{b=0} { if ($0 ~ /^[[:space:]]*$/) { b++; if (b<=1) print; } else { b=0; print } }' \
  "$tmp" > "$file"

# ---------------------------------------------------------------------------
# 同步清理同目录的 index.tsx：删掉引用被删 CSS 类的 JSX 块与不再使用的 import。
# 仅当选择器包含 info/footer 且存在同级 index.tsx 时才执行（squoosh Intro 页面场景）。
# 否则（单纯删 CSS 块）不做任何 JSX 改动，保持脚本通用。
# ---------------------------------------------------------------------------
jsx="$(dirname "$file")/index.tsx"
if [[ -f "$jsx" ]] && [[ " ${selectors[*]} " == *" info "* || " ${selectors[*]} " == *" footer "* ]]; then
  tmp2="$(mktemp)"

  # 删除 <section class={style.info}> ... </section> 块（按 <section>/</section> 标签深度平衡）
  awk '
  {
    o = gsub("<section", "&", $0)
    c = gsub("</section>", "&", $0)
    if (skip == 0) {
      if (depth == 0 && $0 ~ /^[[:space:]]*<section class=\{style\.info\}>/) {
        skip = 1; depth = o - c; next
      }
      print; depth += o - c
    } else {
      depth += o - c
      if (depth <= 0) skip = 0
    }
  }' "$jsx" > "$tmp2" && mv "$tmp2" "$jsx"

  tmp2="$(mktemp)"
  # 删除 <footer class={style.footer}> ... </footer> 块（<footer> 可嵌套，按标签深度平衡）
  awk '
  {
    o = gsub("<footer", "&", $0)
    c = gsub("</footer>", "&", $0)
    if (skip == 0) {
      if (depth == 0 && $0 ~ /^[[:space:]]*<footer class=\{style\.footer\}>/) {
        skip = 1; depth = o - c; next
      }
      print; depth += o - c
    } else {
      depth += o - c
      if (depth <= 0) skip = 0
    }
  }' "$jsx" > "$tmp2" && mv "$tmp2" "$jsx"

  # 删除因上述 JSX 块被删而失效的 import 行（这些标识符已无引用）
  sed -i \
    -e "/^import githubLogo from 'url:\.\/imgs\/github-logo\.svg';/d" \
    -e "/^import smallSectionAsset from 'url:\.\/imgs\/info-content\/small\.svg';/d" \
    -e "/^import simpleSectionAsset from 'url:\.\/imgs\/info-content\/simple\.svg';/d" \
    -e "/^import secureSectionAsset from 'url:\.\/imgs\/info-content\/secure\.svg';/d" \
    -e "/^import SlideOnScroll from '.\/SlideOnScroll';/d" \
    "$jsx"

  # 收紧多余空行
  awk 'BEGIN{b=0} { if ($0 ~ /^[[:space:]]*$/) { b++; if (b<=1) print; } else { b=0; print } }' \
    "$jsx" > "$tmp2" && mv "$tmp2" "$jsx"

  echo "已同步清理 $jsx 中引用 info/footer 的 JSX 块与无用 import"
fi

if [ "$removed" -gt 0 ]; then
  echo "已删除 ${removed} 个顶层块 (选择器: ${selectors[*]}) -> $file"
else
  echo "未找到目标选择器 (${selectors[*]})，文件未改动。"
fi
