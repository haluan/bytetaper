FROM envoyproxy/envoy:v1.30-latest

# Copy the configuration file and set ownership to the 'envoy' user
COPY --chown=envoy:envoy examples/envoy/envoy.yaml /etc/envoy/envoy.yaml

# Explicitly switch to the non-root 'envoy' user
USER envoy

# The default ENTRYPOINT and CMD will use /etc/envoy/envoy.yaml
