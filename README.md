# StereoWidener
Plugin to do stereo widening with decorrelation.

Stereo widening widens the perception of the stereo image by boosting the surrounds and lowering the mid/side ratio. This plugin has controls for the stereo width in lower and higher frequencies, with a controllable cutoff frequency.
The filterbank consists of a lowpass and highpass in cascade, which can be energy preserving (Butterworth, default), or amplitude preserving (Linkwitz-Riley). Two options for decorrelation are available -- efficient convolution with velvet noise filters (default), or a cascade of allpass filters with randomised phases. A transient handling block ensures that transients are not smeared by the decorrelating filters.

<p align = "center">
  <img width="300" alt="Screen Shot 2025-05-10 at 12 22 00 PM" src="https://github.com/user-attachments/assets/ce87589d-6be5-4515-a3b0-ed3e945ba577" />
</p>

### For MacOS users
Use the installer included with the release. The next time you restart your DAW the plugin should show up under the developer name **orchi**.

### Theory
The details of this plugin are outlined in the paper <a href = "https://scholar.google.com/scholar_url?url=https://www.dafx.de/paper-archive/2024/papers/DAFx24_paper_92.pdf&hl=en&sa=T&oi=gsb-gga&ct=res&cd=0&d=12685130807317541840&ei=ijcfaNfVLaWFieoP0K2MsAk&scisig=AAZF9b8UP3zZeFDkTPpbaPV7uWJ2"><b> An open source stereo widening plugin </b></a> - Orchisama Das in Proc. of International Conference on Digital Audio Effects, DAFx 2024.
