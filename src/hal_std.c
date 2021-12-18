#include "hal_std.h"

void f2d_init(field2d_t* field, field_size_t width, field_size_t height, nh_radius_t nh_radius) {
    field->width = width;
    field->height = height;
    field->ticks_count = 0x00;
    field->evol_step = DEFAULT_EVOL_STEP;
    field->pulse_window = DEFAULT_PULSE_WINDOW;

    field->nh_radius = nh_radius;
    field->fire_threshold = DEFAULT_THRESHOLD;
    field->recovery_value = DEFAULT_RECOVERY_VALUE;
    field->charge_value = DEFAULT_CHARGE_VALUE;
    field->decay_value = DEFAULT_DECAY_RATE;
    field->syngen_pulses_count = DEFAULT_SYNGEN_PULSE * DEFAULT_PULSE_WINDOW;
    field->max_syn_count = DEFAULT_MAX_TOUCH * SNH_COUNT(SNH_DIAM(nh_radius));

    field->neurons = (neuron_t*) malloc(field->width * field->height * sizeof(neuron_t));

    for (field_size_t y = 0; y < field->height; y++) {
        for (field_size_t x = 0; x < field->width; x++) {
            field->neurons[IDX2D(x, y, field->width)].nh_mask = DEFAULT_NH_MASK;
            field->neurons[IDX2D(x, y, field->width)].value = DEFAULT_STARTING_VALUE;
            field->neurons[IDX2D(x, y, field->width)].syn_count = 0x00u;
            field->neurons[IDX2D(x, y, field->width)].pulse_mask = 0x00000000u;
            field->neurons[IDX2D(x, y, field->width)].pulse = 0x00u;
        }
    }
}

field2d_t* f2d_copy(field2d_t* other) {
    field2d_t* field = (field2d_t*) malloc(sizeof(field2d_t));
    field->width = other->width;
    field->height = other->height;
    field->ticks_count = other->ticks_count;
    field->evol_step = other->evol_step;
    field->pulse_window = other->pulse_window;

    field->nh_radius = other->nh_radius;
    field->fire_threshold = other->fire_threshold;
    field->recovery_value = other->recovery_value;
    field->charge_value = other->charge_value;
    field->decay_value = other->decay_value;
    field->syngen_pulses_count = other->syngen_pulses_count;
    field->max_syn_count = other->max_syn_count;

    field->neurons = (neuron_t*) malloc(field->width * field->height * sizeof(neuron_t));

    for (field_size_t y = 0; y < other->height; y++) {
        for (field_size_t x = 0; x < other->width; x++) {
            field->neurons[IDX2D(x, y, other->width)] = other->neurons[IDX2D(x, y, other->width)];
        }
    }

    return field;
}

void f2d_set_nhradius(field2d_t* field, nh_radius_t radius) {
    // Only set radius if greater than zero.
    if (radius > 0) {
        field->nh_radius = radius;
    }
}

void f2d_set_nhmask(field2d_t* field, nh_mask_t mask) {
    for (field_size_t y = 0; y < field->height; y++) {
        for (field_size_t x = 0; x < field->width; x++) {
            field->neurons[IDX2D(x, y, field->width)].nh_mask = mask;
        }
    }
}

void f2d_set_fire_threshold(field2d_t* field, neuron_threshold_t threshold) {
    field->fire_threshold = threshold;
}

void f2d_set_max_touch(field2d_t* field, float touch) {
    // Only set touch if a valid value is provided.
    if (touch <= 1 && touch >= 0) {
        field->max_syn_count = touch * SNH_COUNT(SNH_DIAM(field->nh_radius));
    }
}

void f2d_set_syngen_beat(field2d_t* field, float beat) {
    // Only set beat if a valid value is provided.
    if (beat <= 1 && beat >= 0) {
        field->max_syn_count = beat * field->pulse_window;
    }
}

void f2d_feed(field2d_t* field, field_size_t starting_index, field_size_t count, neuron_value_t* values) {
    if (starting_index + count < field->width * field->height) {
        // Loop through count.
        for (field_size_t i = starting_index; i < starting_index + count; i++) {
            field->neurons[i].value += values[i];
        }
    }
}

void f2d_dfeed(field2d_t* field, field_size_t starting_index, field_size_t count, neuron_value_t value) {
    if (starting_index + count < field->width * field->height) {
        // Loop through count.
        for (field_size_t i = starting_index; i < starting_index + count; i++) {
            field->neurons[i].value += value;
        }
    }
}

void f2d_rfeed(field2d_t* field, field_size_t starting_index, field_size_t count, neuron_value_t max_value) {
    if (starting_index + count < field->width * field->height) {
        // Loop through count.
        for (field_size_t i = starting_index; i < starting_index + count; i++) {
            field->neurons[i].value += rand() % max_value;
        }
    }
}

void f2d_sfeed(field2d_t* field, field_size_t starting_index, field_size_t count, neuron_value_t value, field_size_t spread) {
    if ((starting_index + count) * spread < field->width * field->height) {
        // Loop through count.
        for (field_size_t i = starting_index; i < starting_index + count; i++) {
            field->neurons[i * spread].value += value;
        }
    }
}

void f2d_rsfeed(field2d_t* field, field_size_t starting_index, field_size_t count, neuron_value_t max_value, field_size_t spread) {
    if ((starting_index + count) * spread < field->width * field->height) {
        // Loop through count.
        for (field_size_t i = starting_index; i < starting_index + count; i++) {
            field->neurons[i * spread].value += rand() % max_value;
        }
    }
}

void f2d_tick(field2d_t* prev_field, field2d_t* next_field) {
    ticks_count_t rand;

    #pragma omp parallel for
    for (field_size_t y = 0; y < prev_field->height; y++) {
        for (field_size_t x = 0; x < prev_field->width; x++) {
            // Retrieve the involved neurons.
            neuron_t prev_neuron = prev_field->neurons[IDX2D(x, y, prev_field->width)];
            neuron_t* next_neuron = &(next_field->neurons[IDX2D(x, y, prev_field->width)]);

            // Copy prev neuron values to the new one.
            *next_neuron = prev_neuron;

            next_neuron->syn_count = 0x00u;

            /* Compute the neighborhood diameter:
                   d = 7
              <------------->
               r = 3
              <----->
              +-|-|-|-|-|-|-+
              |             |
              |             |
              |      X      |
              |             |
              |             |
              +-|-|-|-|-|-|-+
            */
            field_size_t nh_diameter = 2 * prev_field->nh_radius + 1;

            nh_mask_t prev_mask = prev_neuron.nh_mask;

            rand = xorshf96();

            // Increment the current neuron value by reading its connected neighbors.
            for (nh_radius_t j = 0; j < nh_diameter; j++) {
                for (nh_radius_t i = 0; i < nh_diameter; i++) {
                    // Exclude the central neuron from the list of neighbors.
                    if (!(j == prev_field->nh_radius && i == prev_field->nh_radius)) {
                        // Fetch the current neighbor.
                        neuron_t neighbor = prev_field->neurons[IDX2D(WRAP(x + (i - prev_field->nh_radius), prev_field->width),
                                                                      WRAP(y + (j - prev_field->nh_radius), prev_field->height),
                                                                      prev_field->width)];

                        // Check if the last bit of the mask is 1 or zero, 1 = active input, 0 = inactive input.
                        if (prev_mask & 0x01) {
                            if (neighbor.value > prev_field->fire_threshold) {
                                next_neuron->value += DEFAULT_CHARGE_VALUE;
                            }
                            next_neuron->syn_count++;
                        }

                        float nb_pulse = ((float) neighbor.pulse) / ((float) (prev_field->pulse_window));

                        // Perform evolution phase if allowed.
                        // evol_step is incremented by 1 to account for edge cases and human readable behavior:
                        // 0x0000 -> 0 + 1 = 1, so the field evolves at every tick, meaning that there are no free ticks between evolutions.
                        // 0xFFFF -> 65535 + 1 = 65536, so the field never evolves, meaning that there is an infinite amount of ticks between evolutions.
                        if ((prev_field->ticks_count % (prev_field->evol_step + 1)) == 0 &&
                            // (prev_field->ticks_count + (IDX2D(i, j, nh_diameter))) % 1000 < 10) {
                            (rand + (IDX2D(i, j, nh_diameter))) % 1000 < 10) {
                            if (prev_mask & 0x01 &&
                                nb_pulse < DEFAULT_SYNGEN_PULSE) {
                                // Delete synapse.
                                nh_mask_t mask = ~(next_neuron->nh_mask);
                                mask |= (0x01 << IDX2D(i, j, nh_diameter));
                                next_neuron->nh_mask = ~mask;
                            } else if (!(prev_mask & 0x01) &&
                                       nb_pulse > DEFAULT_SYNGEN_PULSE &&
                                       prev_neuron.syn_count < prev_field->max_syn_count) {
                                // Add synapse.
                                next_neuron->nh_mask |= (0x01 << IDX2D(i, j, nh_diameter));
                            }
                        }
                    }

                    // Shift the mask to check for the next neighbor.
                    prev_mask >>= 0x01;
                }
            }

            // Push to equilibrium by decaying to zero, both from above and below.
            if (prev_neuron.value > 0x00) {
                next_neuron->value -= DEFAULT_DECAY_RATE;
            } else if (prev_neuron.value < 0x00) {
                next_neuron->value += DEFAULT_DECAY_RATE;
            }

            // Bring the neuron back to recovery if it just fired, otherwise fire it if its value is over its threshold.
            if (prev_neuron.value > prev_field->fire_threshold) {
                // Fired at the previous step.
                next_neuron->value = DEFAULT_RECOVERY_VALUE;

                // Store pulse.
                next_neuron->pulse_mask |= 0x01;
                next_neuron->pulse++;
            }

            if ((prev_neuron.pulse_mask >> prev_field->pulse_window) & 0x01) {
                // Decrease pulse if the oldest recorded pulse is active.
                next_neuron->pulse--;
            }

            next_neuron->pulse_mask <<= 0x01;
        }
    }

    next_field->ticks_count++;
}