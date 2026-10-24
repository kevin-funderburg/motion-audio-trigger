# Audio Samples

Audio is grouped by theme. Each theme has an `examples/` directory for
shareable samples included in the repository and a `local/` directory for
locally supplied samples ignored by Git.

The layout below shows the existing Halloween theme and how a future Christmas
theme would fit:

```text
media/audio/
├── README.md
├── halloween/
│   ├── examples/
│   └── local/
└── christmas/       # Possible future theme
    ├── examples/
    └── local/
```

`halloween/examples/` includes the `The-creature` sample as a working example.
Place your own MP3 or WAV files directly in the relevant theme's `local/`
directory, creating it if necessary. The ignore rule covers `local/` directories
under every theme. Keep files that should not be distributed in `local/`.

Choose a theme when starting the server from the repository root:

```bash
python3 server.py -t halloween
python3 server.py --theme christmas
```

The default is `halloween`. The chosen theme's directory must already exist
under `media/audio/`; unknown themes are rejected at startup. To add a theme,
create its `examples/` and `local/` directories and start the server with its
name.

For each `POST /motion`, the server gathers all MP3 and WAV files recursively
under the selected theme and randomly selects one. Example and local samples
share the same selection pool, and samples placed directly in the theme or
in nested directories are included. Filename extensions are case-insensitive.

The directories are checked on every request, so adding or removing samples
does not require a server restart. Consecutive requests may select the same
sample. If the theme contains no samples, the endpoint returns `500`.
