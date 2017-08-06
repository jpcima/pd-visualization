# pd-visualization

This pd-visualization package provides Pure Data externals for visualizing signals.

These visualizers run in external processes and do not disturb real-time signal processing.

![patch](docs/patch.png)

The bang message is used to show or hide the visualizer window. By default the visualizer is not started.
When started and in the hidden state, the visualizers do not consume any CPU time.

Platform compatibility:
- Linux
- Windows
- probably Darwin, testing is needed

## wfvisu~

This visualization shows a waterfall view of the DFT of the input.

![wfvisu~](docs/screen-wfvisu~.png)

## sgvisu~

This visualization shows a spectrogram of the DFT of the input.

![sgvisu~](docs/screen-sgvisu~.png)

## ogvisu~

This visualization shows an oscillogram of the input.

Features:
- collapsible UI controls
- control of position and scale
- trigger

![ogvisu~](docs/screen-ogvisu~.png)
