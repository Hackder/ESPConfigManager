const char *IndexPage = R"=====(
<!DOCTYPE html>
<html lang="en">
  <head>
    <meta charset="UTF-8" />
    <meta http-equiv="X-UA-Compatible" content="IE=edge" />
    <meta name="viewport" content="width=device-width, initial-scale=1.0" />
    <link
      rel="stylesheet"
      href="https://unpkg.com/@picocss/pico@1.*/css/pico.min.css"
    />
    <script src="//unpkg.com/alpinejs" defer></script>
    <title>ESP Config</title>
  </head>
  <body>
    <nav class="container-fluid">
      <ul>
        <li>
          <strong>ESP Config</strong>
        </li>
      </ul>
    </nav>
    <main
      class="container"
      x-data="{ loading: true, error: false, schema: [], config: {} }"
      x-init="Promise.all([
        fetch('/configSchema').then(res => res.json()).then(data => schema = data),
        fetch('/config').then(res => res.json()).then(data => config = data)])
        .finally(() => loading = false).catch(err => { error = true; console.error(err) })"
    >
      <h1 x-bind:aria-busy="loading">Configuration</h1>
      <template x-if="error">
        <p style="color: red">There was an error loading the configuration!</p>
      </template>
      <template x-if="!loading && !error">
        <form
          x-data="{ submitting: false, error: false }"
          @submit.prevent="submitting = true;
            fetch('/config', { method: 'POST', body: JSON.stringify(config) })
              .catch(err => error = true)
              .finally(() => submitting = false)"
        >
          <template x-for="configItem in schema">
            <label
              x-bind:for="configItem.name"
              x-bind:style="configItem.type === 'bool' ? 'margin-bottom: var(--spacing)' : ''"
            >
              <template x-if="configItem.type !== 'bool'">
                <span x-text="configItem.name"></span>
              </template>
              <template x-if="configItem.type === 'string'">
                <input
                  type="text"
                  x-bind:name="configItem.name"
                  x-bind:id="configItem.name"
                  x-model="config[configItem.name]"
                  required
                />
              </template>
              <template x-if="configItem.type === 'int'">
                <input
                  type="number"
                  x-bind:name="configItem.name"
                  x-bind:id="configItem.name"
                  min="-2147483648"
                  max="2147483647"
                  x-model.number="config[configItem.name]"
                  required
                />
              </template>
              <template x-if="configItem.type === 'bool'">
                <input
                  type="checkbox"
                  role="switch"
                  x-bind:name="configItem.name"
                  x-bind:id="configItem.name"
                  x-model="config[configItem.name]"
                />
              </template>
              <template x-if="configItem.type === 'bool'">
                <span x-text="configItem.name"></span>
              </template>
            </label>
          </template>

          <p x-show="error" style="color: red">Error saving configuration!</p>

          <button type="submit" x-bind:aria-busy="submitting">Save</button>
        </form>
      </template>
    </main>
  </body>
</html>

)=====";