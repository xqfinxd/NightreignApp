/**
 * Show a toast message that auto-dismisses after 1 second
 * @param {string} message - The message to display
 * @param {number} duration - Duration in milliseconds (default: 1000)
 */
function showToast(message, duration = 1000) {
  const toast = document.getElementById('toast');
  if (!toast) return;
  
  toast.textContent = message;
  toast.classList.add('show');
  
  setTimeout(() => {
    toast.classList.remove('show');
  }, duration);
}

 /**
     * Initialize a bottom-centered toggle group:
     * - Click to toggle `.is-active`
     * - Programmatic selection via `selectToggle(...)`
     *
     * @param {string|Element} groupRef - selector or element for the group container
     * @param {Object} [opts]
     * @param {string} [opts.activeClass='is-active'] - class to mark active button
     * @returns {{ select: Function, getValue: Function, onChange: Function, destroy: Function }}
     */
    function initToggleGroup(groupRef, opts = {}) {
      const group = typeof groupRef === 'string' ? document.querySelector(groupRef) : groupRef;
      if (!group) throw new Error('Toggle group not found');
      const activeClass = opts.activeClass || 'is-active';

      const buttons = () => Array.from(group.querySelectorAll('.btn'));

      // Click handler: set active & emit change
      const onClick = (e) => {
        const btn = e.target.closest('.btn');
        if (!btn || btn.disabled) return;

        setActive(btn);
        emitChange(btn);
      };

      group.addEventListener('click', onClick);

      function setActive(target, { focus = false, emit = false } = {}) {
        const list = buttons();
        list.forEach(b => b.classList.remove(activeClass));
        target.classList.add(activeClass);
        if (focus) target.focus();
        if (emit) emitChange(target);
      }

      function emitChange(btn) {
        // Standard CustomEvent with .detail.value
        const value = btn.dataset.value ?? buttons().indexOf(btn);
        group.dispatchEvent(new CustomEvent('change', {
          bubbles: true,
          detail: { value, button: btn }
        }));
      }

      /**
       * Programmatically select a button.
       * @param {string|number|Element} selector - data-value (string), index (number), or the button element
       * @param {Object} [options]
       * @param {boolean} [options.emit=true] - whether to dispatch a 'change' event
       * @param {boolean} [options.focus=false] - whether to focus the button
       * @returns {HTMLElement|null} - the selected button or null if not found
       */
      function select(selector, { emit = true, focus = false } = {}) {
        const list = buttons();
        let btn = null;

        if (typeof selector === 'number') {
          selector = selector.toString();
        }

        btn = list.find(b => b.dataset.value === selector) || null;

        if (!btn || btn.disabled) return null;
        setActive(btn, { focus, emit });
        return btn;
      }

      /** Get current value (prefers data-value, otherwise index) */
      function getValue() {
        const list = buttons();
        const current = list.find(b => b.classList.contains(activeClass));
        if (!current) return null;
        return current.dataset.value ?? list.indexOf(current);
      }

      /** Subscribe to changes (returns unsubscribe) */
      function onChange(handler) {
        const wrapped = (e) => handler(e.detail);
        group.addEventListener('change', wrapped);
        return () => group.removeEventListener('change', wrapped);
      }

      /** Cleanup listeners */
      function destroy() {
        group.removeEventListener('click', onClick);
      }

      return { select, getValue, onChange, destroy };
    }

    // Map buttons - simple click handlers without toggle state
    const mapGroup = document.querySelector('.js-toggle-group-map');
    if (mapGroup) {
      // Remove any active states from map buttons (they shouldn't have toggle state)
      mapGroup.querySelectorAll('.btn').forEach(btn => {
        btn.classList.remove('is-active');
        
        btn.addEventListener('click', function() {
          const value = this.dataset.value;
          console.log('Map button clicked:', value, 'button text:', this.textContent.trim());
          
          // Call C++ filterMap function
          if (typeof Module !== 'undefined' && Module.filterMap) {
            const mapIndex = parseInt(value);
            console.log('Calling filterMap with index:', mapIndex);
            Module.filterMap(mapIndex);
          }
        });
      });
    }

    // Boss buttons - simple click handlers without toggle state
    const bossGroup = document.querySelector('.js-toggle-group-boss');
    if (bossGroup) {
      // Remove any active states from boss buttons (they shouldn't have toggle state)
      bossGroup.querySelectorAll('.btn').forEach(btn => {
        btn.classList.remove('is-active');
        
        btn.addEventListener('click', function() {
          const value = this.dataset.value;
          console.log('Boss button clicked:', value, 'button text:', this.textContent.trim());
          
          // Call C++ filterNightlord function
          if (typeof Module !== 'undefined' && Module.filterNightlord) {
            const nightlordIndex = parseInt(value);
            console.log('Calling filterNightlord with index:', nightlordIndex);
            Module.filterNightlord(nightlordIndex);
          }
        });
      });
    }

    // ===== Collapsible Master Button Logic =====
    function initCollapsibleButton(masterId, groupSelector) {
      const masterBtn = document.getElementById(masterId);
      const fabGroup = document.querySelector(groupSelector);
      
      if (!masterBtn || !fabGroup) {
        console.warn('Master button or FAB group not found:', masterId, groupSelector);
        return null;
      }

      let isExpanded = false;

      masterBtn.addEventListener('click', function(e) {
        e.stopPropagation();
        isExpanded = !isExpanded;
        
        // Close other groups first
        document.querySelectorAll('.fab-group').forEach(group => {
          if (group !== fabGroup) {
            group.classList.add('collapsed');
          }
        });
        document.querySelectorAll('.fab-master-btn').forEach(btn => {
          if (btn !== masterBtn) {
            btn.classList.remove('expanded');
            btn.setAttribute('aria-expanded', 'false');
          }
        });
        
        if (isExpanded) {
          fabGroup.classList.remove('collapsed');
          masterBtn.classList.add('expanded');
          masterBtn.setAttribute('aria-expanded', 'true');
        } else {
          fabGroup.classList.add('collapsed');
          masterBtn.classList.remove('expanded');
          masterBtn.setAttribute('aria-expanded', 'false');
        }
      });

      // Auto-collapse when any button is clicked
      fabGroup.addEventListener('click', function(e) {
        if (e.target.classList.contains('btn') && isExpanded) {
          setTimeout(function() {
            isExpanded = false;
            fabGroup.classList.add('collapsed');
            masterBtn.classList.remove('expanded');
            masterBtn.setAttribute('aria-expanded', 'false');
          }, 200);
        }
      });

      return {
        collapse: function() {
          isExpanded = false;
          fabGroup.classList.add('collapsed');
          masterBtn.classList.remove('expanded');
          masterBtn.setAttribute('aria-expanded', 'false');
        }
      };
    }

    // Initialize both collapsible buttons
    const mapCollapse = initCollapsibleButton('fab-toggle-map', '.js-toggle-group-map');
    const bossCollapse = initCollapsibleButton('fab-toggle-boss', '.js-toggle-group-boss');

    // Close on outside click
    document.addEventListener('click', function(e) {
      const isClickInside = e.target.closest('.fab-container');
      if (!isClickInside) {
        if (mapCollapse) mapCollapse.collapse();
        if (bossCollapse) bossCollapse.collapse();
      }
    });

    // B1 Overlay toggle button
    const b1OverlayBtn = document.getElementById('btn-b1-overlay');
    if (b1OverlayBtn) {
      b1OverlayBtn.addEventListener('click', function() {
        if (typeof Module !== 'undefined' && Module.toggleB1Overlay) {
          Module.toggleB1Overlay();
          this.classList.toggle('active');
        }
      });
    }

    // Later, if needed:
    // unsubscribeMap();
    // toggleMap.destroy();