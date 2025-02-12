# ratio-core

This is the core library for the Ratio framework. It provides the basic utilities for managing components, lists, server connections, and user sessions. The core library is designed to be lightweight and flexible, making it easy to integrate into various web applications. It provides essential utilities for handling:

 - **Component management** – A structured approach to defining and interacting with components.
 - **List management** – Built-in support for managing collections of components.
 - **Server connection** – Tools for handling communication with a server.
 - **User management** – Basic utilities for managing the current user session.

## Installation

To install the core library, you can use npm:

```bash
npm install ratio-core
```

## Usage

The core library provides a set of utilities that can be used to build web applications. Here is an example of how to use the core library to create a simple component:

```javascript
import { AppComponent } from 'ratio-core';

class MyApp extends AppComponent {

    count = 0;
    button = document.createElement('button');

    constructor() {
        super();
        button.textContent = 'Click me';
        button.onclick = () => this.increment();
        this.element.appendChild(button);
    }

    increment() {
        this.count++;
        this.button.textContent = `Clicked ${this.count} times`;
    }
}

const myComponent = new MyApp();
```