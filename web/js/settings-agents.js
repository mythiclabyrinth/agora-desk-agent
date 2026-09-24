// Settings > Agents: one form per bridge.
function field(label, value, opts = {}) {
  const wrap = el('label', '', label);
  const input = el('input');
  input.value = value || '';
  input.type = opts.type || 'text';
  input.autocomplete = 'off';
  input.maxLength = opts.max || 180;
  input.placeholder = opts.placeholder || '';
  if (opts.required) input.required = true;
  if (opts.pattern) input.pattern = opts.pattern;
  wrap.append(input);
  if (opts.help) wrap.append(el('small', '', opts.help));
  return { wrap, input };
}
function activateAgentForm() {
  document
    .querySelectorAll('[data-agent-form]')
    .forEach((f) => (f.hidden = f.dataset.agentForm !== agentTab));
  document.querySelectorAll('[data-agent-tab]').forEach((b) => {
    b.classList.toggle('on', b.dataset.agentTab === agentTab);
    b.setAttribute('aria-pressed', String(b.dataset.agentTab === agentTab));
  });
}
function renderAgents() {
  if (!agents.length) {
    $('#agent-forms').replaceChildren(
      el('p', 'form-empty', 'Agent settings will appear when the board connects.'),
    );
    return;
  }
  settingsBuilt = true;
  const root = $('#agent-forms');
  root.replaceChildren();
  if (!agents.some((a) => a.id === agentTab)) agentTab = agents[0].id;
  const tabs = el('div', 'sub agents');
  agents.forEach((a) => {
    const b = button(a.name || a.title, '', () => {
      agentTab = a.id;
      activateAgentForm();
    });
    b.dataset.agentTab = a.id;
    tabs.append(b);
  });
  root.append(tabs);
  agents.forEach((agent) => {
    const form = el('form', 'panel');
    form.dataset.agentForm = agent.id;
    const head = el('div', 'panel-heading');
    head.append(
      el('h3', '', (agent.name || agent.title) + ' connection'),
      el('p', '', 'Use the same details as the bridge running on your computer.'),
    );
    const name = field('Display name', agent.name, { max: 40, required: true, placeholder: agent.title });
    const agentId = field('Agent ID', agent.agent_id, {
      max: 64,
      pattern: '[A-Za-z0-9_\\-]+',
      help: 'Use the agent identifier from your Agora setup.',
      placeholder: 'claude-cli',
    });
    const url = field('Agora URL', agent.url, {
      type: 'url',
      required: true,
      placeholder: 'http://192.168.1.20:4470',
      help: 'The server address, without an /api path.',
    });
    const channel = field('Channel ID', agent.channel, {
      max: 80,
      required: true,
      pattern: '[A-Za-z0-9_\\-]+',
      placeholder: 'general-a1b2',
    });
    const token = field('Access token', '', {
      type: 'password',
      max: 500,
      required: !agent.token_set,
      placeholder: agent.token_set ? 'Saved — leave blank to keep it' : 'Paste your Agora access token',
      help: 'Leave blank to keep your saved token.',
    });
    token.input.autocomplete = 'new-password';
    const pair = el('div', 'field-pair');
    pair.append(name.wrap, agentId.wrap);
    const row = el('div', 'row');
    const status = el('p', 'note');
    status.setAttribute('role', 'status');
    const save = el('button', 'primary', 'Save connection');
    save.type = 'submit';
    row.append(status, save);
    form.append(head, pair, url.wrap, channel.wrap, token.wrap, row);
    form.addEventListener('submit', async (e) => {
      e.preventDefault();
      save.disabled = true;
      note(status, 'Saving connection…');
      try {
        const data = await api('/api/agents', {
          id: agent.id,
          name: name.input.value.trim(),
          agent_id: agentId.input.value.trim(),
          url: url.input.value.trim(),
          channel: channel.input.value.trim(),
          token: token.input.value.trim(),
        });
        note(
          status,
          data.ready ? 'Connection saved. Ready when you are.' : 'Saved. Complete the remaining details.',
        );
        status.classList.add('good');
        token.input.value = '';
        token.input.required = false;
        token.input.placeholder = 'Saved — leave blank to keep it';
        const title = name.input.value.trim() || agent.title;
        tabs.querySelector('[data-agent-tab="' + agent.id + '"]').textContent = title;
        head.querySelector('h3').textContent = title + ' connection';
        Object.assign(agents.find((a) => a.id === agent.id) || agent, {
          name: title,
          ready: data.ready,
          token_set: true,
          url: url.input.value.trim(),
          channel: channel.input.value.trim(),
          agent_id: agentId.input.value.trim(),
        });
        refreshStatus();
      } catch (err) {
        note(status, err.message, true);
      } finally {
        save.disabled = false;
      }
    });
    root.append(form);
  });
  activateAgentForm();
}
