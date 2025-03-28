import { ComputePositionConfig } from '@floating-ui/dom';

declare module 'cytoscape-popper' {

  interface PopperOptions extends ComputePositionConfig {
  }

  interface PopperInstance {
    update(): void;
    destroy(): void;
  }
}

import cytoscape from 'cytoscape';
import dagre from 'cytoscape-dagre';
import cytoscapePopper, { PopperInstance, PopperOptions, RefElement } from 'cytoscape-popper';
import {
  computePosition,
  flip,
  shift,
  limitShift
} from '@floating-ui/dom';

export { solver } from './solver/solver';
export { SolverGraph } from './solver/solver_graph';
export { TimelinesChart } from './solver/solver_timelines';
export * from './solver/solver_components';

function popperFactory(ref: RefElement, content: HTMLElement, options?: PopperOptions): PopperInstance {
  const popperOptions = { middleware: [flip(), shift({ limiter: limitShift() })], ...options, };

  function update() {
    computePosition(ref, content, popperOptions).then(({ x, y }) => {
      Object.assign(content.style, {
        left: `${x}px`,
        top: `${y}px`,
      });
    });
  }

  function destroy() {
    content.remove();
  }

  update();
  return { update, destroy };
}

cytoscape.use(dagre);
cytoscape.use(cytoscapePopper(popperFactory));

import { state_variable } from "./solver/state_variable";
import { reusable_resource } from "./solver/reusable_resource";
import { consumable_resource } from "./solver/consumable_resource";
import { solver } from './solver/solver';

solver.timeline.TimelineManager.get_instance().add_timeline_generator(new state_variable.StateVariableTimelineGenerator());
solver.timeline.TimelineManager.get_instance().add_timeline_generator(new reusable_resource.ReusableResourceTimelineGenerator());
solver.timeline.TimelineManager.get_instance().add_timeline_generator(new consumable_resource.ConsumableResourceTimelineGenerator());

solver.chart.ChartManager.get_instance().add_chart_generator(new state_variable.StateVariableChartGenerator());
solver.chart.ChartManager.get_instance().add_chart_generator(new reusable_resource.ReusableResourceChartGenerator());
solver.chart.ChartManager.get_instance().add_chart_generator(new consumable_resource.ConsumableResourceChartGenerator());