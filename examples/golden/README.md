Golden Images

Place reference PNGs here for CI visual regression:

- directional-light-test.png
- spot-light-test.png

How to generate locally (Linux example):

1) Build release binary via CI steps or locally.
2) Render examples with fixed size:

   - Directional:
     builds/desktop/cmake/glint --ops examples/json-ops/directional-light-test.json \
       --render examples/golden/directional-light-test.png --w 256 --h 256

   - Spot:
     builds/desktop/cmake/glint --ops examples/json-ops/spot-light-test.json \
       --render examples/golden/spot-light-test.png --w 256 --h 256

Commit the PNGs so CI can compare outputs to these goldens. The CI tolerances are:
- Primary metric SSIM >= 0.995 (fallback to RMSE normalized <= 0.004).

