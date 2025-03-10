const char* IndexPage = R"=====(
<!DOCTYPE html>
<html lang="en">

<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>ESP Config Manager</title>
  <link rel="stylesheet" href="https://cdn.jsdelivr.net/npm/@picocss/pico@2/css/pico.min.css">
  <link rel="stylesheet" href="https://cdn.jsdelivr.net/npm/@picocss/pico@2/css/pico.colors.min.css">
  <link rel="stylesheet" href="https://cdn.jsdelivr.net/npm/notyf@3/notyf.min.css">
  <style>
    main {
      margin-top: 2rem;
    }
  </style>

</head>

<body>
  <script src="https://cdn.jsdelivr.net/npm/notyf@3/notyf.min.js"></script>
  <script type="module">
    import {h, render} from 'https://esm.sh/preact';
    import {useState, useEffect} from 'https://esm.sh/preact/hooks';

    const baseUrl = window.location.protocol === 'file:' ? 'http://192.168.88.92' : '';

    const notyf = new Notyf();

    function ButtonField({field, config, setConfig, change}) {
      const [loading, setLoading] = useState(false);

      async function handleClick() {
        setLoading(true);
        const resp = await fetch(baseUrl + '/event', {
          method: 'POST',
          headers: {
            'Content-Type': 'application/json'
          },
          body: JSON.stringify({
            name: field.name,
            event: 'click',
            config: config.reduce((acc, field) => {
              acc[field.name] = field.value;
              return acc;
            }, {})
          })
        });

        if (resp.ok) {
          const newConfig = await resp.json();
          setConfig(newConfig);
          setLoading(false);
          notyf.success('Event sent successfully');
        } else {
          alert('Failed to send event');
        }
      }

      const title = field.options.title || field.name;

      return h('button', {type: 'submit', role: 'button', onclick: handleClick, ariaBusy: loading, disabled: loading}, title);
    }

    function Field({field, config, setConfig, change}) {

      if (field.type === 'button') {
        return h(ButtonField, {field, config, setConfig, change});
      }

      if (field.type === 'int' || field.type === 'float') {
        const hasRange = field.options.min !== undefined && field.options.max !== undefined;
        const numericChange = (value) => {
          if (field.type === 'int') {
            value = parseInt(value);
          } else {
            value = parseFloat(value);
          }
          change(value);
        };
        return h('label', null,
          field.options.title || field.name,
          hasRange && h('small', {class: 'pico-color-grey-500'}, ` (${field.options.min} - ${field.options.max})`),
          h('input', {
            type: 'number', name: field.name, min: field.options.min, max: field.options.max, step: field.options.step, value: field.value,
            style: hasRange ? 'margin-bottom: 0' : '',
            onchange: (e) => numericChange(e.target.value)
          }),
          hasRange && h('input', {
            type: 'range', name: field.name, min: field.options.min, max: field.options.max, step: field.options.step, value: field.value,
            sytle: 'margin-bottom: var(--pico-spacing)',
            oninput: (e) => numericChange(e.target.value)
          }),
          h('small', null, field.options.description)
        );
      }

      if (field.type === 'string') {
        return h('label', null,
          field.options.title || field.name,
          h('input', {type: 'text', name: field.name, class: 'form-control', value: field.value, onchange: (e) => change(e.target.value)}),
          h('small', null, field.options.description)
        );
      }

      if (field.type === 'bool') {
        return h('fieldset', null,
          h('label', null,
            h('input', {type: 'checkbox', name: field.name, role: "switch", checked: field.value, onchange: (e) => change(e.target.checked)}),
            field.options.title || field.name,
            h('small', {style: 'margin-top: 0em; margin-left: 0.1em;'}, field.options.description)
          )
        );
      }

      return h('div', null, "Unknown field type: ", field.type);
    }

    function App() {
      const [config, setConfig] = useState(null);
      const [loading, setLoading] = useState(true);

      useEffect(() => {
        async function fetchData() {
          const response = await fetch(baseUrl + '/config');
          const data = await response.json();
          setConfig(data);
          setLoading(false);
        }
        fetchData();
      }, []);

      const handleChange = (index, value) => {
        const newConfig = [...config];
        newConfig[index].value = value;
        setConfig(newConfig);
      };

      if (loading) {
        return h('main', {class: 'container'}, h('h1', null, 'Loading...'));
      }

      return h('main', {class: 'container'},
        h('h1', null, 'Configuration'),
        h('form', {onsubmit: (e) => e.preventDefault()},
          config.map((field, index) =>
            h(Field, {
              field,
              config,
              setConfig,
              change: (value) => handleChange(index, value)
            })
          )
        )
      );
    }

    render(h(App, null), document.body);
  </script>

</body>

</html>
)=====";
