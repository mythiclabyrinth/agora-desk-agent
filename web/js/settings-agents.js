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
function channelOption(value, text) {
  const opt = el('option', '', text);
  opt.value = value;
  return opt;
}
// A channel select filled from Agora, with a typed field for servers that cannot list channels.
// `form` holds the other inputs, read unsaved so channels can be listed before the first save.
function channelPicker(agent, form) {
  const wrap = el('div', 'channel-field');
  const pickRow = el('div', 'channel-row');
  const pickLabel = el('label', '', 'Channel');
  const select = el('select');
  select.required = true;
  const load = button('Load channels', 'ghost');
  pickLabel.append(select);
  pickRow.append(pickLabel, load);
  const typed = field('Channel ID', '', {
    max: 80,
    pattern: '[A-Za-z0-9_\\-]+',
    placeholder: 'general-a1b2',
  });
  const status = el('p', 'note');
  status.setAttribute('role', 'status');
  const toggle = button('Type an ID instead', 'text-button');
  const foot = el('div', 'channel-foot');
  foot.append(status, toggle);
  wrap.append(pickRow, typed.wrap, foot);
  let typing = false;

  const saved = () => (agents.find((a) => a.id === agent.id) || agent).channel || '';
  function remember(id) {
    if (id && ![...select.options].some((o) => o.value === id)) select.prepend(channelOption(id, id));
    if (id) select.value = id;
  }
  function showTyping(on) {
    typing = on;
    pickRow.hidden = on;
    typed.wrap.hidden = !on;
    select.required = !on;
    typed.input.required = on;
    toggle.textContent = on ? 'Pick from the list' : 'Type an ID instead';
    if (on && select.value) typed.input.value = select.value;
    if (!on && typed.input.checkValidity()) remember(typed.input.value.trim());
    (on ? typed.input : select).focus();
  }
  function fill(data) {
    const who = data.agent.name || form.name.value.trim() || agent.title;
    const keep = saved();
    const groups = new Map();
    data.channels.forEach((c) => {
      const group = c.group || 'Channels';
      if (!groups.has(group)) groups.set(group, []);
      groups.get(group).push(c);
    });
    select.replaceChildren();
    let firstMember = '';
    groups.forEach((list, group) => {
      const og = el('optgroup');
      og.label = group;
      [...list.filter((c) => c.member), ...list.filter((c) => !c.member)].forEach((c) => {
        const text =
          (c.kind === 'agent_dm' ? c.name || c.id : '#' + (c.name || c.id)) +
          (c.member ? ' · ' + who + ' is here' : '');
        og.append(channelOption(c.id, text));
        if (c.member && !firstMember) firstMember = c.id;
      });
      select.append(og);
    });
    if (keep && !data.channels.some((c) => c.id === keep))
      select.prepend(channelOption(keep, keep + ' (saved)'));
    const pick = keep || firstMember;
    if (!pick)
      select.prepend(channelOption('', data.channels.length ? 'Choose a channel' : 'No channels yet'));
    select.value = pick;
    return who;
  }
  load.addEventListener('click', async () => {
    load.disabled = true;
    note(status, 'Loading channels…');
    try {
      const data = await api(
        '/api/agents/' + agent.id + '/channels',
        {
          url: form.url.value.trim(),
          agent_id: form.agentId.value.trim(),
          name: form.name.value.trim(),
          token: form.token.value.trim(),
        },
        30000,
      );
      const who = fill(data);
      note(status, who + '’s bridge is ' + (data.agent.live ? 'online.' : 'offline.'));
    } catch (err) {
      note(status, err.message, true);
    } finally {
      load.disabled = false;
    }
  });
  toggle.addEventListener('click', () => showTyping(!typing));
  if (saved()) select.append(channelOption(saved(), saved()));
  else select.append(channelOption('', 'Load channels to choose one'));
  typed.wrap.hidden = true;
  return {
    wrap,
    value: () => (typing ? typed.input.value : select.value).trim(),
    remember,
  };
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
    const token = field('Access token', '', {
      type: 'password',
      max: 500,
      required: !agent.token_set,
      placeholder: agent.token_set ? 'Saved — leave blank to keep it' : 'Paste your Agora access token',
      help: 'Leave blank to keep your saved token.',
    });
    token.input.autocomplete = 'new-password';
    const channel = channelPicker(agent, {
      name: name.input,
      agentId: agentId.input,
      url: url.input,
      token: token.input,
    });
    const pair = el('div', 'field-pair');
    pair.append(name.wrap, agentId.wrap);
    const row = el('div', 'row');
    const status = el('p', 'note');
    status.setAttribute('role', 'status');
    const save = el('button', 'primary', 'Save connection');
    save.type = 'submit';
    row.append(status, save);
    form.append(head, pair, url.wrap, token.wrap, channel.wrap, row);
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
          channel: channel.value(),
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
          channel: channel.value(),
          agent_id: agentId.input.value.trim(),
        });
        channel.remember(channel.value());
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
