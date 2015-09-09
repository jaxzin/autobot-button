# Autobot Button

An Arduino/Bluefruit Micro button and status light designed for triggering an action with the tap of a large button and
receiving feedback.

# Usage

There are seven states the button can be in, and 1 action that can be performed.

## States

### Disconnected

### Connected

### Pending
![Pending Light](docs/images/autobot_pending.gif)

### Good
![Pending Light](docs/images/autobot_good.gif)

### Warning

### Error

### Unknown
![Pending Light](docs/images/autobot_unknown.gif)

## Actions

There is currently only one action that can be performed by the button, pressing it. Multiple presses will send multiple
press events to the listener, it is up to the listener to ignore presses if an action is currently pending.


# Contributing

## Circuit
TODO: Diagram and instructions to come

## Programming the Bluefruit Micro
Use the Arduino Development Environment to upload `deploy_button.ino`

