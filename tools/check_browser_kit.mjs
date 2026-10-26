// Private release check: every selection links identically with GNU ld.
import {readFileSync, writeFileSync, mkdirSync} from 'node:fs';
import {resolve, dirname} from 'node:path';
import {execFileSync} from 'node:child_process';
import assert from 'node:assert/strict';
import {openKit, linkKit, sha256} from '../builder/core.mjs';

const [kitFile, objectDir, ld, objcopy] = process.argv.slice(2);
const kit = await openKit(JSON.parse(readFileSync(kitFile, 'utf8')));
const destination = resolve(dirname(kitFile), 'reference'); mkdirSync(destination);
const layout = resolve('linker/opxy-1.2.0.ld');
const results = [];
let index = 0;
for (let mask = 1; mask < 2 ** kit.machines.length; mask++) {
  const selected = kit.machines.filter((_, i) => mask & 2 ** i).map(m => m.id);
  for (let extMask = 0; extMask < 2 ** (kit.extensions?.length ?? 0); extMask++) {
    const extensions = (kit.extensions ?? []).filter((_, i) => extMask & 2 ** i).map(e => e.id);
    const linked = linkKit(kit, selected, null, extensions), prefix = resolve(destination, String(++index));
    const definitions = [...Object.entries(linked.definitions), ...linked.aliases.entries()]
      .map(([key, value]) => `--defsym=${key}=0x${value.toString(16)}`);
    execFileSync(ld, ['-T', layout, '-e', 'extensions_synth_exit', '-o', prefix + '.elf', ...definitions,
      ...linked.objects.map(o => resolve(objectDir, o.name))], {stdio: 'pipe', windowsHide: true});
    execFileSync(objcopy, ['-O', 'binary', prefix + '.elf', prefix + '.bin'], {stdio: 'pipe', windowsHide: true});
    const actual = readFileSync(prefix + '.bin');
    assert.deepEqual(Buffer.from(linked.module), actual, 'GNU linker mismatch: ' + selected.join(', '));
    // Export exact symbol addresses for independent Python LDR assembly too.
    const result = {selected, extensions, sizes: linked.sizes, globals: Object.fromEntries(linked.globals),
      module_sha256: await sha256(linked.module)};
    writeFileSync(prefix + '.json', JSON.stringify(result)); results.push(result);
    console.log('GNU link parity:', selected.join(' + '), 'extensions:', extensions.join(',') || 'none', linked.sizes.allocated, 'bytes');
  }
}
writeFileSync(resolve(destination, 'selections.json'), JSON.stringify(results, null, 2));
