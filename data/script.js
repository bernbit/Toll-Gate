// ============================================================================
// CONFIGURATION
// ============================================================================

// ESP32 IP - will auto-detect current host
const ESP32_IP = window.location.hostname || "192.168.4.1";
const BASE_URL = `http://${ESP32_IP}`;

// ============================================================================
// GLOBAL VARIABLES
// ============================================================================

let eventSource = null;
let currentEnrollmentId = null;

// Vehicle-specific variables
let editingVehicleRFID = null;
let vehicleEventSource = null;

// ============================================================================
// PAGE INITIALIZATION
// ============================================================================

document.addEventListener("DOMContentLoaded", function () {
  console.log("Page loaded, ESP32 IP:", ESP32_IP);

  // Load fingerprint list on page load
  loadFingerprintList();

  // Add event listener to the "Add Fingerprint" button
  const addButton = document.querySelector(".main-header .add-fingerprint-btn");
  if (addButton) {
    addButton.addEventListener("click", openFingerprintModal);
  }

  // Add event listener for form submission
  const form = document.getElementById("fingerprintForm");
  if (form) {
    form.addEventListener("submit", submitFingerprint);
  }

  // Vehicle-specific initialization
  if (window.location.pathname.includes("vehicle.html")) {
    loadVehicleList();

    const addVehicleBtn = document.querySelector(".add-vehicle-btn");
    if (addVehicleBtn) {
      addVehicleBtn.addEventListener("click", openVehicleModal);
    }

    const deleteAllVehiclesBtn = document.querySelector(
      ".deleteAll-vehicles-btn"
    );
    if (deleteAllVehiclesBtn) {
      deleteAllVehiclesBtn.addEventListener("click", deleteAllVehicles);
    }
  }
});

// ============================================================================
// SIDEBAR FUNCTIONS
// ============================================================================

function toggleSidebar() {
  const sidebar = document.getElementById("sidebar");
  const mainContent = document.querySelector(".content");
  sidebar.classList.toggle("minimized");
  mainContent.classList.toggle("minimized");
}

// Close sidebar on small screens when clicking outside
document.addEventListener("click", function (event) {
  const sidebar = document.getElementById("sidebar");
  const mainContent = document.querySelector(".content");
  const toggleBtn = document.getElementById("toggleBtn");

  if (
    window.innerWidth < 768 &&
    !sidebar.contains(event.target) &&
    !sidebar.classList.contains("minimized")
  ) {
    sidebar.classList.add("minimized");
    mainContent.classList.add("minimized");
  }
});

// ============================================================================
// FINGERPRINT LIST FUNCTIONS
// ============================================================================

async function loadFingerprintList() {
  try {
    console.log("Loading fingerprint list...");
    const response = await fetch(`${BASE_URL}/fp/list`);

    if (!response.ok) {
      throw new Error(`HTTP error! status: ${response.status}`);
    }

    const fingerprints = await response.json();
    console.log("Loaded fingerprints:", fingerprints);

    // Update table
    const tableBody = document.querySelector(".fingerprint-table tbody");
    tableBody.innerHTML = ""; // Clear existing rows

    if (fingerprints.length === 0) {
      const emptyRow = document.createElement("tr");
      emptyRow.innerHTML = `
        <td colspan="4" style="text-align: center; color: #999; padding: 40px;">
          No fingerprints registered yet. Click the + button to add one.
        </td>
      `;
      tableBody.appendChild(emptyRow);
    } else {
      fingerprints.forEach((fp) => {
        const row = document.createElement("tr");
        row.innerHTML = `
          <td>${fp.id}</td>
          <td>${fp.owner}</td>
          <td>${fp.role}</td>
          <td class="table-action-btn">
            <button class="edit-fingerprint-btn" onclick="editFingerprint(${fp.id}, '${fp.owner}', '${fp.role}')">
              <svg width="30" height="30" viewBox="0 0 30 30" fill="none" xmlns="http://www.w3.org/2000/svg">
                <g clip-path="url(#clip0_44_202)">
                  <path d="M15 30C23.2843 30 30 23.2843 30 15C30 6.71573 23.2843 0 15 0C6.71573 0 0 6.71573 0 15C0 23.2843 6.71573 30 15 30Z" fill="#26A1F4"/>
                  <path d="M20.2236 13.9183L20.2154 13.9101L16.081 9.77283C16.081 9.77283 11.042 14.8119 8.61385 17.2904C8.3115 17.5986 8.0824 18.0275 7.94646 18.4424C7.54803 19.6605 7.21814 20.9033 6.85135 22.1338C6.75291 22.4636 6.77283 22.7543 7.03123 23.0004C7.27498 23.2347 7.5492 23.2429 7.8656 23.148C9.03748 22.7965 10.217 22.4619 11.3941 22.125C11.9994 21.9495 12.5482 21.6184 12.9855 21.1646C15.2988 18.8373 20.2236 13.9183 20.2236 13.9183Z" fill="white"/>
                  <path d="M22.6371 8.77792L21.2232 7.36406C20.8606 7.00178 20.3691 6.79828 19.8565 6.79828C19.344 6.79828 18.8524 7.00178 18.4898 7.36406L16.8873 8.96484L21.0351 13.1139L22.6377 11.5113C22.9999 11.1487 23.2033 10.6571 23.2031 10.1445C23.203 9.63196 22.9994 9.14042 22.6371 8.77792Z" fill="white"/>
                </g>
              </svg>
            </button>
            <button class="delete-fingerprint-btn" onclick="deleteFingerprint(${fp.id})">
              <svg width="30" height="30" viewBox="0 0 30 30" fill="none" xmlns="http://www.w3.org/2000/svg">
                <path fill-rule="evenodd" clip-rule="evenodd" d="M15 0C6.72902 0 0 6.72902 0 15C0 23.271 6.72885 30 15 30C23.2712 30 30 23.271 30 15C30 6.72896 23.2711 0 15 0ZM12.0749 6.69897C12.075 6.56966 12.1263 6.44564 12.2177 6.35418C12.3091 6.26273 12.4331 6.21131 12.5624 6.21123H17.4374C17.5668 6.21152 17.6908 6.26309 17.7822 6.35462C17.8736 6.44616 17.925 6.57019 17.9251 6.69955V7.90523H12.0749V6.69897ZM20.0906 23.0911C20.0781 23.2813 19.9934 23.4595 19.8539 23.5892C19.7143 23.719 19.5304 23.7905 19.3399 23.7891H10.6035C10.4131 23.7886 10.2298 23.716 10.0906 23.586C9.95144 23.456 9.86659 23.2781 9.85313 23.0881L9.10588 12.1523H20.888L20.0906 23.0911ZM22.0324 11.1621H7.96758V10.029C7.96783 9.7285 8.08729 9.44041 8.29974 9.22794C8.51218 9.01547 8.80026 8.89598 9.10072 8.8957L20.8991 8.89535C21.1996 8.89577 21.4876 9.01536 21.7001 9.22789C21.9125 9.44042 22.0319 9.72854 22.0322 10.029L22.0324 11.1621ZM12.7354 21.1821V14.2383C12.7354 14.107 12.7876 13.9811 12.8805 13.8882C12.9734 13.7954 13.0993 13.7433 13.2306 13.7433C13.362 13.7434 13.4879 13.7956 13.5807 13.8885C13.6735 13.9814 13.7256 14.1073 13.7256 14.2386V21.1821C13.7266 21.2478 13.7145 21.313 13.6901 21.3739C13.6657 21.4349 13.6294 21.4904 13.5833 21.5372C13.5372 21.584 13.4823 21.6211 13.4217 21.6465C13.3611 21.6718 13.2961 21.6849 13.2305 21.6849C13.1648 21.6849 13.0998 21.6718 13.0392 21.6465C12.9786 21.6211 12.9237 21.584 12.8776 21.5372C12.8316 21.4904 12.7953 21.4349 12.7708 21.3739C12.7464 21.313 12.7343 21.2478 12.7354 21.1821ZM16.2681 21.1821V14.2383C16.2701 14.1083 16.3232 13.9843 16.4159 13.8931C16.5086 13.8019 16.6334 13.7509 16.7634 13.7509C16.8934 13.7509 17.0182 13.8021 17.1108 13.8934C17.2034 13.9846 17.2564 14.1086 17.2583 14.2386V21.1824C17.2598 21.2483 17.248 21.3139 17.2238 21.3752C17.1995 21.4365 17.1633 21.4924 17.1172 21.5395 C17.0711 21.5867 17.016 21.6241 16.9552 21.6497C16.8944 21.6752 16.8291 21.6884 16.7632 21.6884C16.6973 21.6884 16.632 21.6752 16.5712 21.6497C16.5104 21.6241 16.4553 21.5867 16.4092 21.5395C16.3631 21.4924 16.3269 21.4365 16.3026 21.3752C16.2784 21.3139 16.2667 21.248 16.2681 21.1821Z" fill="#FC0005"/>
                <path fill-rule="evenodd" clip-rule="evenodd" d="M20.0906 23.0911C20.0781 23.2813 19.9934 23.4595 19.8539 23.5892C19.7143 23.719 19.5304 23.7905 19.3399 23.7891H10.6035C10.4131 23.7886 10.2298 23.716 10.0906 23.586C9.95144 23.456 9.86659 23.2781 9.85313 23.0881L9.10588 12.1523H20.888L20.0906 23.0911ZM12.7354 14.2383V21.1821C12.7343 21.2478 12.7464 21.313 12.7708 21.3739C12.7953 21.4349 12.8316 21.4904 12.8776 21.5372C12.9237 21.584 12.9786 21.6211 13.0392 21.6465C13.0998 21.6718 13.1648 21.6849 13.2305 21.6849C13.2961 21.6849 13.3611 21.6718 13.4217 21.6465C13.4823 21.6211 13.5372 21.584 13.5833 21.5372C13.6294 21.4904 13.6657 21.4349 13.6901 21.3739C13.7145 21.313 13.7266 21.2478 13.7256 21.1821V14.2386C13.7256 14.1073 13.6735 13.9814 13.5807 13.8885C13.4879 13.7956 13.362 13.7434 13.2306 13.7433C13.0993 13.7433 12.9734 13.7954 12.8805 13.8882C12.7876 13.9811 12.7354 14.107 12.7354 14.2383ZM16.2681 14.2383V21.1821C16.2667 21.248 16.2784 21.3139 16.3026 21.3752C16.3269 21.4365 16.3631 21.4924 16.4092 21.5395C16.4553 21.5867 16.5104 21.6241 16.5712 21.6497C16.632 21.6752 16.6973 21.6884 16.7632 21.6884C16.8291 21.6884 16.8944 21.6752 16.9552 21.6497C17.016 21.6241 17.0711 21.5867 17.1172 21.5395C17.1633 21.4924 17.1995 21.4365 17.2238 21.3752C17.248 21.3139 17.2598 21.2483 17.2583 21.1824V14.2386C17.2564 14.1086 17.2034 13.9846 17.1108 13.8934C17.0182 13.8021 16.8934 13.7509 16.7634 13.7509C16.6334 13.7509 16.5086 13.8019 16.4159 13.8931C16.3232 13.9843 16.2701 14.1083 16.2681 14.2383Z" fill="white"/>
              </svg>
              </button>
          </td>`;
        tableBody.appendChild(row);
      });
    }
  } catch (error) {
    console.error("Error loading fingerprint list:", error);
    // showError("Failed to load fingerprint list. Check ESP32 connection.");
  }
}

// ============================================================================
// MODAL FUNCTIONS
// ============================================================================

function openFingerprintModal() {
  const modal = document.getElementById("fingerprintModal");
  modal.classList.add("active");
  document.body.style.overflow = "hidden";

  // Reset form
  document.getElementById("fingerprintForm").reset();
  const submitBtn = document.getElementById("submitBtn");
  submitBtn.disabled = true;

  // Reset scan status to ready state
  const scanStatus = document.getElementById("scanStatus");
  const scanText = scanStatus.querySelector(".scan-text");
  scanStatus.className = "scan-status";
  scanText.textContent = "Click 'Start Scan' button when ready";

  // Show the start scan button
  const startScanBtn = document.getElementById("startScanBtn");
  if (startScanBtn) {
    startScanBtn.style.display = "block";
    startScanBtn.disabled = false;
  }

  // Do NOT start enrollment automatically
  // User must click the "Start Scan" button
}

function closeFingerprintModal() {
  const modal = document.getElementById("fingerprintModal");
  modal.classList.remove("active");
  document.body.style.overflow = "";

  // Close SSE connection
  if (eventSource) {
    eventSource.close();
    eventSource = null;
  }

  // Reset modal title
  const modalTitle = modal.querySelector(".modal-header h2");
  modalTitle.innerHTML = `
    <svg width="24" height="24" viewBox="0 0 30 30" fill="none" xmlns="http://www.w3.org/2000/svg">
      <path d="M9.3888 24.765C8.48128 23.1374 8.00133 21.1425 8.00133 18.9825C8.00133 15.3524 11.1438 12.3975 14.9988 12.3975C18.8613 12.3975 21.9963 15.3525 21.9963 18.9825C21.9963 19.395 22.3338 19.7325 22.7463 19.7325C23.1589 19.7325 23.4964 19.395 23.4964 18.9825C23.4964 14.5199 19.6863 10.8974 14.9989 10.8974C10.3114 10.8974 6.50128 14.5275 6.50128 18.9825C6.50128 21.3975 7.04881 23.6549 8.0838 25.4999C9.0963 27.3075 9.80133 28.14 11.0988 29.4524C11.2488 29.6024 11.4363 29.6774 11.6313 29.6774C11.8188 29.6774 12.0138 29.6099 12.1488 29.4524C12.4488 29.1674 12.4488 28.6874 12.1563 28.3949C11.0013 27.2325 10.3563 26.49 9.3888 24.765Z" fill="#005C2B"/>
    </svg>
    Register Fingerprint
  `;

  // Show scanner section again
  const scannerSection = document.querySelector(".form-group:has(#scanStatus)");
  if (scannerSection) {
    scannerSection.style.display = "block";
  }

  // Reset scan status
  const scanStatus = document.getElementById("scanStatus");
  const scanText = scanStatus.querySelector(".scan-text");
  scanStatus.className = "scan-status";
  scanText.textContent = "Click 'Start Scan' button when ready";

  // Show/reset start scan button
  const startScanBtn = document.getElementById("startScanBtn");
  if (startScanBtn) {
    startScanBtn.style.display = "block";
    startScanBtn.disabled = false;
    startScanBtn.textContent = "Start Scan";
  }

  // Reset submit button
  document.getElementById("submitBtn").textContent = "Register Fingerprint";

  currentEnrollmentId = null;
  editingFingerprintId = null;
}

// Close modal when clicking outside
document.addEventListener("click", function (event) {
  const modal = document.getElementById("fingerprintModal");
  if (event.target === modal) {
    closeFingerprintModal();
  }
});

// Close modal with Escape key
document.addEventListener("keydown", function (event) {
  if (event.key === "Escape") {
    closeFingerprintModal();
  }
});

// ============================================================================
// FINGERPRINT ENROLLMENT
// ============================================================================

// This function is called when user clicks "Start Scan" button
async function startFingerprintScan() {
  const startScanBtn = document.getElementById("startScanBtn");
  const scanStatus = document.getElementById("scanStatus");
  const scanText = scanStatus.querySelector(".scan-text");

  // Disable start button
  startScanBtn.disabled = true;
  startScanBtn.textContent = "Scanning...";

  scanStatus.className = "scan-status scanning";
  scanText.textContent = "Initializing scanner...";

  // Start the enrollment process
  await startFingerprintEnrollment();
}

async function startFingerprintEnrollment() {
  const startScanBtn = document.getElementById("startScanBtn");

  try {
    // Get list of existing fingerprints to find next available ID
    const response = await fetch(`${BASE_URL}/fp/list`);
    const existingFingerprints = await response.json();

    // Extract existing IDs
    const existingIds = existingFingerprints.map((fp) => fp.id);

    // Find next available ID (1-127)
    let nextId = 1;
    while (existingIds.includes(nextId) && nextId <= 127) {
      nextId++;
    }

    if (nextId > 127) {
      showScanError("Fingerprint storage is full (max 127)");
      // Re-enable start button
      if (startScanBtn) {
        startScanBtn.disabled = false;
        startScanBtn.textContent = "Start Scan";
        startScanBtn.style.display = "block";
      }
      return;
    }

    console.log("Starting enrollment for ID:", nextId);
    currentEnrollmentId = nextId;

    // Hide start scan button once enrollment begins
    if (startScanBtn) {
      startScanBtn.style.display = "none";
    }

    // Setup SSE connection FIRST
    setupSSEConnection();

    // Small delay to ensure SSE is connected
    await new Promise((resolve) => setTimeout(resolve, 500));

    // Start enrollment on ESP32
    const enrollResponse = await fetch(`${BASE_URL}/fp/enroll?id=${nextId}`);
    const enrollText = await enrollResponse.text();
    console.log("Enrollment started:", enrollText);
  } catch (error) {
    console.error("Error starting enrollment:", error);
    showScanError("Cannot connect to ESP32. Check WiFi connection.");
    // Re-enable start button on error
    if (startScanBtn) {
      startScanBtn.disabled = false;
      startScanBtn.textContent = "Start Scan";
      startScanBtn.style.display = "block";
    }
  }
}

function setupSSEConnection() {
  const scanStatus = document.getElementById("scanStatus");
  const scanText = scanStatus.querySelector(".scan-text");

  // Close existing connection if any
  if (eventSource) {
    eventSource.close();
  }

  console.log("Setting up SSE connection...");
  scanStatus.className = "scan-status scanning";
  scanText.textContent = "Connecting to scanner...";

  eventSource = new EventSource(`${BASE_URL}/events`);

  eventSource.addEventListener("open", () => {
    console.log("SSE connection opened");
  });

  eventSource.addEventListener("status", (e) => {
    console.log("Status:", e.data);
    scanStatus.className = "scan-status scanning";
    scanText.textContent = e.data;
  });

  eventSource.addEventListener("prompt", (e) => {
    console.log("Prompt:", e.data);
    scanStatus.className = "scan-status scanning";
    scanText.textContent = "👆 " + e.data;
  });

  eventSource.addEventListener("error", (e) => {
    console.log("Error event:", e.data);
    scanStatus.className = "scan-status error";
    scanText.textContent = "❌ " + e.data;
    document.getElementById("submitBtn").disabled = true;

    // Show start scan button again on error
    const startScanBtn = document.getElementById("startScanBtn");
    if (startScanBtn) {
      startScanBtn.style.display = "block";
      startScanBtn.disabled = false;
      startScanBtn.textContent = "Start Scan";
    }
  });

  eventSource.addEventListener("done", (e) => {
    console.log("Done:", e.data);
    scanStatus.className = "scan-status success";
    scanText.textContent = "✓ " + e.data;

    // Enable submit button
    document.getElementById("submitBtn").disabled = false;
    document.getElementById("fingerprintId").value = currentEnrollmentId;

    // Close SSE connection
    if (eventSource) {
      eventSource.close();
      eventSource = null;
    }
  });

  eventSource.onerror = (error) => {
    console.error("SSE Error:", error);

    // Check if connection was intentionally closed (after success)
    if (eventSource && eventSource.readyState === EventSource.CLOSED) {
      console.log("SSE connection closed");
      return;
    }

    // Only show error if we haven't completed enrollment
    if (document.getElementById("submitBtn").disabled) {
      showScanError("Connection to scanner lost. Please try again.");
    }
  };
}

function showScanError(message) {
  const scanStatus = document.getElementById("scanStatus");
  const scanText = scanStatus.querySelector(".scan-text");
  scanStatus.className = "scan-status error";
  scanText.textContent = "❌ " + message;
  document.getElementById("submitBtn").disabled = true;

  // Also show notification
  showNotification("Scan Error", message, "error");
}

// Close modals with Escape key
document.addEventListener("keydown", function (event) {
  if (event.key === "Escape") {
    closeNotification();
    closeConfirmation(false);
    closeFingerprintModal();
  }
});

// ============================================================================
// FORM SUBMISSION
// ============================================================================

async function submitFingerprint(event) {
  event.preventDefault();

  const fingerprintId = document.getElementById("fingerprintId").value;
  const owner = document.getElementById("owner").value;
  const role = document.getElementById("role").value;

  if (!fingerprintId) {
    showNotification(
      "Missing Information",
      "Fingerprint ID is required. Please scan a fingerprint first.",
      "error"
    );
    return;
  }

  console.log("Submitting fingerprint data:", { fingerprintId, owner, role });

  try {
    const response = await fetch(`${BASE_URL}/fp/save`, {
      method: "POST",
      headers: {
        "Content-Type": "application/json",
      },
      body: JSON.stringify({
        id: parseInt(fingerprintId),
        owner: owner,
        role: role,
      }),
    });

    if (!response.ok) {
      throw new Error("Failed to save fingerprint data");
    }

    const result = await response.json();
    console.log("Save result:", result);

    closeFingerprintModal();
    await loadFingerprintList();

    if (editingFingerprintId) {
      showNotification(
        "Updated Successfully",
        `Fingerprint information has been updated.\n\nID: ${fingerprintId}\nOwner: ${owner}\nRole: ${role}`,
        "success"
      );
      editingFingerprintId = null;
    } else {
      showNotification(
        "Registered Successfully",
        `New fingerprint has been registered.\n\nID: ${fingerprintId}\nOwner: ${owner}\nRole: ${role}`,
        "success"
      );
    }
  } catch (error) {
    console.error("Error saving fingerprint:", error);
    showNotification(
      "Save Failed",
      "Unable to save fingerprint data. Please try again.",
      "error"
    );
  }
}
// ============================================================================
// UTILITY FUNCTIONS
// ============================================================================

// ============================================================================
// NOTIFICATION SYSTEM
// ============================================================================

function showNotification(title, message, type = "success") {
  const modal = document.getElementById("notificationModal");
  const titleEl = document.getElementById("notificationTitle");
  const messageEl = document.getElementById("notificationMessage");
  const iconEl = document.getElementById("notificationIcon");

  // Set content
  titleEl.textContent = title;
  messageEl.textContent = message;

  // Set icon based on type
  iconEl.className = `notification-icon ${type}`;

  if (type === "success") {
    iconEl.innerHTML = `
      <svg width="48" height="48" viewBox="0 0 24 24" fill="none" xmlns="http://www.w3.org/2000/svg">
        <circle cx="12" cy="12" r="10" stroke="#22b14d" stroke-width="2"/>
        <path d="M8 12l2 2 4-4" stroke="#22b14d" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"/>
      </svg>
    `;
  } else if (type === "error") {
    iconEl.innerHTML = `
      <svg width="48" height="48" viewBox="0 0 24 24" fill="none" xmlns="http://www.w3.org/2000/svg">
        <circle cx="12" cy="12" r="10" stroke="#e74c3c" stroke-width="2"/>
        <path d="M15 9l-6 6M9 9l6 6" stroke="#e74c3c" stroke-width="2" stroke-linecap="round"/>
      </svg>
    `;
  }

  // Show modal
  modal.classList.add("active");
  document.body.style.overflow = "hidden";
}

function closeNotification() {
  const modal = document.getElementById("notificationModal");
  modal.classList.remove("active");
  document.body.style.overflow = "";
}

function showConfirmation(title, message) {
  return new Promise((resolve) => {
    const modal = document.getElementById("confirmationModal");
    const titleEl = document.getElementById("confirmationTitle");
    const messageEl = document.getElementById("confirmationMessage");

    titleEl.textContent = title;
    messageEl.textContent = message;

    modal.classList.add("active");
    document.body.style.overflow = "hidden";

    // Store resolver for callback
    window.confirmationResolver = resolve;
  });
}

function closeConfirmation(result) {
  const modal = document.getElementById("confirmationModal");
  modal.classList.remove("active");
  document.body.style.overflow = "";

  if (window.confirmationResolver) {
    window.confirmationResolver(result);
    window.confirmationResolver = null;
  }
}

// Legacy support - replace old alert functions
function showError(message) {
  showNotification("Error", message, "error");
}

function showSuccess(message) {
  showNotification("Success", message, "success");
}

// Optional: Add delete functionality (you can add delete buttons to table later)
async function deleteFingerprint(id) {
  const confirmed = await showConfirmation(
    "Delete Fingerprint",
    `Are you sure you want to delete fingerprint ID ${id}?\n\nThis action cannot be undone.`
  );

  if (!confirmed) {
    return;
  }

  try {
    console.log("Deleting fingerprint ID:", id);
    const response = await fetch(`${BASE_URL}/fp/delete?id=${id}`);

    if (response.ok) {
      showNotification(
        "Deleted Successfully",
        `Fingerprint ID ${id} has been removed from the system.`,
        "success"
      );
      await loadFingerprintList();
    } else {
      throw new Error("Failed to delete fingerprint");
    }
  } catch (error) {
    console.error("Error deleting fingerprint:", error);
    showNotification(
      "Delete Failed",
      "Unable to delete fingerprint. Please check your connection and try again.",
      "error"
    );
  }
}

// ============================================================================
// EDIT FINGERPRINT FUNCTION
// ============================================================================

let editingFingerprintId = null;

function editFingerprint(id, currentOwner, currentRole) {
  editingFingerprintId = id;

  // Open modal
  const modal = document.getElementById("fingerprintModal");
  modal.classList.add("active");
  document.body.style.overflow = "hidden";

  // Change modal title
  const modalTitle = modal.querySelector(".modal-header h2");
  modalTitle.innerHTML = `
    <svg width="24" height="24" viewBox="0 0 30 30" fill="none" xmlns="http://www.w3.org/2000/svg">
      <g clip-path="url(#clip0_44_202)">
        <path d="M15 30C23.2843 30 30 23.2843 30 15C30 6.71573 23.2843 0 15 0C6.71573 0 0 6.71573 0 15C0 23.2843 6.71573 30 15 30Z" fill="#26A1F4"/>
        <path d="M20.2236 13.9183L20.2154 13.9101L16.081 9.77283C16.081 9.77283 11.042 14.8119 8.61385 17.2904C8.3115 17.5986 8.0824 18.0275 7.94646 18.4424C7.54803 19.6605 7.21814 20.9033 6.85135 22.1338C6.75291 22.4636 6.77283 22.7543 7.03123 23.0004C7.27498 23.2347 7.5492 23.2429 7.8656 23.148C9.03748 22.7965 10.217 22.4619 11.3941 22.125C11.9994 21.9495 12.5482 21.6184 12.9855 21.1646C15.2988 18.8373 20.2236 13.9183 20.2236 13.9183Z" fill="white"/>
        <path d="M22.6371 8.77792L21.2232 7.36406C20.8606 7.00178 20.3691 6.79828 19.8565 6.79828C19.344 6.79828 18.8524 7.00178 18.4898 7.36406L16.8873 8.96484L21.0351 13.1139L22.6377 11.5113C22.9999 11.1487 23.2033 10.6571 23.2031 10.1445C23.203 9.63196 22.9994 9.14042 22.6371 8.77792Z" fill="white"/>
      </g>
    </svg>
    Edit Fingerprint Information
  `;

  // Fill form with current data
  document.getElementById("owner").value = currentOwner;
  document.getElementById("role").value = currentRole;
  document.getElementById("fingerprintId").value = id;

  // Hide fingerprint scanner section
  const scannerSection = document.querySelector(".form-group:has(#scanStatus)");
  if (scannerSection) {
    scannerSection.style.display = "none";
  }

  // Enable submit button immediately (no need to scan)
  document.getElementById("submitBtn").disabled = false;
  document.getElementById("submitBtn").textContent = "Update Information";
}

// ============================================================================
// DELETE SINGLE FINGERPRINT
// ============================================================================

async function deleteFingerprint(id) {
  if (
    !confirm(
      `Are you sure you want to delete fingerprint ID ${id}?\n\nThis action cannot be undone.`
    )
  ) {
    return;
  }

  try {
    console.log("Deleting fingerprint ID:", id);
    const response = await fetch(`${BASE_URL}/fp/delete?id=${id}`);

    if (response.ok) {
      showSuccess(`Fingerprint ID ${id} deleted successfully`);
      await loadFingerprintList();
    } else {
      throw new Error("Failed to delete fingerprint");
    }
  } catch (error) {
    console.error("Error deleting fingerprint:", error);
    showError("Failed to delete fingerprint. Please try again.");
  }
}

// ============================================================================
// DELETE ALL FINGERPRINTS
// ============================================================================

async function deleteAllFingerprints() {
  const confirmed = await showConfirmation(
    "⚠️ Delete All Fingerprints",
    "This will permanently delete ALL fingerprints from the system.\n\nThis action CANNOT be undone.\n\nAre you absolutely sure?"
  );

  if (!confirmed) {
    return;
  }

  // Double confirmation for critical action
  const doubleCheck = await showConfirmation(
    "Final Confirmation",
    "Please confirm one more time.\n\nDelete ALL fingerprints?"
  );

  if (!doubleCheck) {
    return;
  }

  try {
    console.log("Deleting all fingerprints...");
    const response = await fetch(`${BASE_URL}/fp/deleteall`);

    if (response.ok) {
      const result = await response.text();
      showNotification(
        "All Deleted",
        "All fingerprints have been successfully removed from the system.",
        "success"
      );
      await loadFingerprintList();
    } else {
      throw new Error("Failed to delete all fingerprints");
    }
  } catch (error) {
    console.error("Error deleting all fingerprints:", error);
    showNotification(
      "Delete Failed",
      "Unable to delete all fingerprints. Please check your connection and try again.",
      "error"
    );
  }
}

// Add event listener for delete all button
document.addEventListener("DOMContentLoaded", function () {
  const deleteAllBtn = document.querySelector(".deleteAll-fingerprints-btn");
  if (deleteAllBtn) {
    deleteAllBtn.addEventListener("click", deleteAllFingerprints);
  }
});

// ============================================================================
// VEHICLE LIST FUNCTIONS
// ============================================================================

async function loadVehicleList() {
  try {
    console.log("Loading vehicle list...");
    const response = await fetch(`${BASE_URL}/vehicle/list`);

    if (!response.ok) {
      throw new Error(`HTTP error! status: ${response.status}`);
    }

    const vehicles = await response.json();
    console.log("Loaded vehicles:", vehicles);

    const tableBody = document.querySelector(".vehicle-table tbody");
    tableBody.innerHTML = "";

    if (vehicles.length === 0) {
      const emptyRow = document.createElement("tr");
      emptyRow.innerHTML = `
        <td colspan="9" style="text-align: center; color: #999; padding: 40px;">
          No vehicles registered yet. Click the + button to add one.
        </td>
      `;
      tableBody.appendChild(emptyRow);
    } else {
      vehicles.forEach((vehicle) => {
        const row = document.createElement("tr");
        row.innerHTML = `
          <td>${vehicle.rfid}</td>
          <td>${vehicle.plateNo}</td>
          <td>${vehicle.type}</td>
          <td>${vehicle.owner}</td>
          <td>${vehicle.role}</td>
          <td>${vehicle.year}</td>
          <td>${vehicle.section}</td>
          <td>${vehicle.course}</td>
          <td class="table-action-btn">
            <button class="edit-vehicle-btn" onclick="editVehicle('${vehicle.rfid}', '${vehicle.plateNo}', '${vehicle.type}', '${vehicle.owner}', '${vehicle.role}', '${vehicle.year}', '${vehicle.section}', '${vehicle.course}')">
            <svg
                    width="30"
                    height="30"
                    viewBox="0 0 30 30"
                    fill="none"
                    xmlns="http://www.w3.org/2000/svg"
                  >
                    <g clip-path="url(#clip0_44_202)">
                      <path
                        d="M15 30C23.2843 30 30 23.2843 30 15C30 6.71573 23.2843 0 15 0C6.71573 0 0 6.71573 0 15C0 23.2843 6.71573 30 15 30Z"
                        fill="#26A1F4"
                      />
                      <path
                        d="M20.2236 13.9183L20.2154 13.9101L16.081 9.77283C16.081 9.77283 11.042 14.8119 8.61385 17.2904C8.3115 17.5986 8.0824 18.0275 7.94646 18.4424C7.54803 19.6605 7.21814 20.9033 6.85135 22.1338C6.75291 22.4636 6.77283 22.7543 7.03123 23.0004C7.27498 23.2347 7.5492 23.2429 7.8656 23.148C9.03748 22.7965 10.217 22.4619 11.3941 22.125C11.9994 21.9495 12.5482 21.6184 12.9855 21.1646C15.2988 18.8373 20.2236 13.9183 20.2236 13.9183Z"
                        fill="white"
                      />
                      <path
                        d="M22.6371 8.77792L21.2232 7.36406C20.8606 7.00178 20.3691 6.79828 19.8565 6.79828C19.344 6.79828 18.8524 7.00178 18.4898 7.36406L16.8873 8.96484L21.0351 13.1139L22.6377 11.5113C22.9999 11.1487 23.2033 10.6571 23.2031 10.1445C23.203 9.63196 22.9994 9.14042 22.6371 8.77792Z"
                        fill="white"
                      />
                    </g>
                  </svg>
            </button>
            <button class="delete-vehicle-btn" onclick="deleteVehicle('${vehicle.rfid}')">
                  <svg
                    width="30"
                    height="30"
                    viewBox="0 0 30 30"
                    fill="none"
                    xmlns="http://www.w3.org/2000/svg"
                  >
                    <path
                      fill-rule="evenodd"
                      clip-rule="evenodd"
                      d="M15 0C6.72902 0 0 6.72902 0 15C0 23.271 6.72885 30 15 30C23.2712 30 30 23.271 30 15C30 6.72896 23.2711 0 15 0ZM12.0749 6.69897C12.075 6.56966 12.1263 6.44564 12.2177 6.35418C12.3091 6.26273 12.4331 6.21131 12.5624 6.21123H17.4374C17.5668 6.21152 17.6908 6.26309 17.7822 6.35462C17.8736 6.44616 17.925 6.57019 17.9251 6.69955V7.90523H12.0749V6.69897ZM20.0906 23.0911C20.0781 23.2813 19.9934 23.4595 19.8539 23.5892C19.7143 23.719 19.5304 23.7905 19.3399 23.7891H10.6035C10.4131 23.7886 10.2298 23.716 10.0906 23.586C9.95144 23.456 9.86659 23.2781 9.85313 23.0881L9.10588 12.1523H20.888L20.0906 23.0911ZM22.0324 11.1621H7.96758V10.029C7.96783 9.7285 8.08729 9.44041 8.29974 9.22794C8.51218 9.01547 8.80026 8.89598 9.10072 8.8957L20.8991 8.89535C21.1996 8.89577 21.4876 9.01536 21.7001 9.22789C21.9125 9.44042 22.0319 9.72854 22.0322 10.029L22.0324 11.1621ZM12.7354 21.1821V14.2383C12.7354 14.107 12.7876 13.9811 12.8805 13.8882C12.9734 13.7954 13.0993 13.7433 13.2306 13.7433C13.362 13.7434 13.4879 13.7956 13.5807 13.8885C13.6735 13.9814 13.7256 14.1073 13.7256 14.2386V21.1821C13.7266 21.2478 13.7145 21.313 13.6901 21.3739C13.6657 21.4349 13.6294 21.4904 13.5833 21.5372C13.5372 21.584 13.4823 21.6211 13.4217 21.6465C13.3611 21.6718 13.2961 21.6849 13.2305 21.6849C13.1648 21.6849 13.0998 21.6718 13.0392 21.6465C12.9786 21.6211 12.9237 21.584 12.8776 21.5372C12.8316 21.4904 12.7953 21.4349 12.7708 21.3739C12.7464 21.313 12.7343 21.2478 12.7354 21.1821ZM16.2681 21.1821V14.2383C16.2701 14.1083 16.3232 13.9843 16.4159 13.8931C16.5086 13.8019 16.6334 13.7509 16.7634 13.7509C16.8934 13.7509 17.0182 13.8021 17.1108 13.8934C17.2034 13.9846 17.2564 14.1086 17.2583 14.2386V21.1824C17.2598 21.2483 17.248 21.3139 17.2238 21.3752C17.1995 21.4365 17.1633 21.4924 17.1172 21.5395C17.0711 21.5867 17.016 21.6241 16.9552 21.6497C16.8944 21.6752 16.8291 21.6884 16.7632 21.6884C16.6973 21.6884 16.632 21.6752 16.5712 21.6497C16.5104 21.6241 16.4553 21.5867 16.4092 21.5395C16.3631 21.4924 16.3269 21.4365 16.3026 21.3752C16.2784 21.3139 16.2667 21.248 16.2681 21.1821Z"
                      fill="#FC0005"
                    />
                    <path
                      fill-rule="evenodd"
                      clip-rule="evenodd"
                      d="M20.0906 23.0911C20.0781 23.2813 19.9934 23.4595 19.8539 23.5892C19.7143 23.719 19.5304 23.7905 19.3399 23.7891H10.6035C10.4131 23.7886 10.2298 23.716 10.0906 23.586C9.95144 23.456 9.86659 23.2781 9.85313 23.0881L9.10588 12.1523H20.888L20.0906 23.0911ZM12.7354 14.2383V21.1821C12.7343 21.2478 12.7464 21.313 12.7708 21.3739C12.7953 21.4349 12.8316 21.4904 12.8776 21.5372C12.9237 21.584 12.9786 21.6211 13.0392 21.6465C13.0998 21.6718 13.1648 21.6849 13.2305 21.6849C13.2961 21.6849 13.3611 21.6718 13.4217 21.6465C13.4823 21.6211 13.5372 21.584 13.5833 21.5372C13.6294 21.4904 13.6657 21.4349 13.6901 21.3739C13.7145 21.313 13.7266 21.2478 13.7256 21.1821V14.2386C13.7256 14.1073 13.6735 13.9814 13.5807 13.8885C13.4879 13.7956 13.362 13.7434 13.2306 13.7433C13.0993 13.7433 12.9734 13.7954 12.8805 13.8882C12.7876 13.9811 12.7354 14.107 12.7354 14.2383ZM16.2681 14.2383V21.1821C16.2667 21.248 16.2784 21.3139 16.3026 21.3752C16.3269 21.4365 16.3631 21.4924 16.4092 21.5395C16.4553 21.5867 16.5104 21.6241 16.5712 21.6497C16.632 21.6752 16.6973 21.6884 16.7632 21.6884C16.8291 21.6884 16.8944 21.6752 16.9552 21.6497C17.016 21.6241 17.0711 21.5867 17.1172 21.5395C17.1633 21.4924 17.1995 21.4365 17.2238 21.3752C17.248 21.3139 17.2598 21.2483 17.2583 21.1824V14.2386C17.2564 14.1086 17.2034 13.9846 17.1108 13.8934C17.0182 13.8021 16.8934 13.7509 16.7634 13.7509C16.6334 13.7509 16.5086 13.8019 16.4159 13.8931C16.3232 13.9843 16.2701 14.1083 16.2681 14.2383Z"
                      fill="white"
                    />
                  </svg>
            </button>
          </td>`;
        tableBody.appendChild(row);
      });
    }
  } catch (error) {
    console.error("Error loading vehicle list:", error);
    showNotification(
      "Load Failed",
      "Failed to load vehicle list. Check ESP32 connection.",
      "error"
    );
  }
}

// ============================================================================
// VEHICLE MODAL FUNCTIONS
// ============================================================================

function openVehicleModal() {
  const modal = document.getElementById("vehicleModal");
  modal.classList.add("active");
  document.body.style.overflow = "hidden";

  // Reset form
  document.getElementById("vehicleForm").reset();
  document.getElementById("submitVehicleBtn").disabled = true;

  // Reset RFID scan status
  const scanStatus = document.getElementById("rfidScanStatus");
  const scanText = scanStatus.querySelector(".scan-text");
  scanStatus.className = "scan-status";
  scanText.textContent = "Click 'Start Scan' button when ready";

  // Show start scan button
  const startScanBtn = document.getElementById("startRFIDScanBtn");
  if (startScanBtn) {
    startScanBtn.style.display = "block";
    startScanBtn.disabled = false;
  }

  // Reset modal title to "Register Vehicle"
  const modalTitle = document.querySelector("#vehicleModal .modal-header h2");
  modalTitle.innerHTML = `
    <!-- Vehicle icon SVG -->
    Register Vehicle
  `;

  // Show RFID scanner section
  const scannerSection = document.querySelector(
    "#vehicleModal .form-group:has(#rfidScanStatus)"
  );
  if (scannerSection) {
    scannerSection.style.display = "block";
  }

  editingVehicleRFID = null;
}

function closeVehicleModal() {
  const modal = document.getElementById("vehicleModal");
  modal.classList.remove("active");
  document.body.style.overflow = "";

  // Close SSE connection if active
  if (vehicleEventSource) {
    vehicleEventSource.close();
    vehicleEventSource = null;
  }

  editingVehicleRFID = null;
}

// ============================================================================
// RFID SCANNING FUNCTIONS
// ============================================================================

async function startRFIDScan() {
  const startScanBtn = document.getElementById("startRFIDScanBtn");
  const scanStatus = document.getElementById("rfidScanStatus");
  const scanText = scanStatus.querySelector(".scan-text");

  startScanBtn.disabled = true;
  startScanBtn.textContent = "Scanning...";

  scanStatus.className = "scan-status scanning";
  scanText.textContent = "Waiting for RFID tag...";

  try {
    // Start the scan on ESP32
    const response = await fetch(`${BASE_URL}/rfid/startscan`);
    if (!response.ok) {
      throw new Error("Failed to start RFID scan");
    }

    // Setup SSE connection to listen for RFID events
    setupRFIDSSEConnection();
  } catch (error) {
    console.error("Error starting RFID scan:", error);
    showRFIDScanError("Cannot connect to ESP32. Check WiFi connection.");
    startScanBtn.disabled = false;
    startScanBtn.textContent = "Start Scan";
  }
}

function setupRFIDSSEConnection() {
  const scanStatus = document.getElementById("rfidScanStatus");
  const scanText = scanStatus.querySelector(".scan-text");

  if (vehicleEventSource) {
    vehicleEventSource.close();
  }

  console.log("Setting up RFID SSE connection...");
  vehicleEventSource = new EventSource(`${BASE_URL}/events`);

  vehicleEventSource.addEventListener("rfid", (e) => {
    console.log("RFID detected:", e.data);

    // Extract RFID from message (format: "Vehicle detected: XXXX")
    const rfidMatch = e.data.match(/Vehicle detected: ([A-F0-9]+)/i);
    if (rfidMatch) {
      const rfid = rfidMatch[1];

      scanStatus.className = "scan-status success";
      scanText.textContent = `✓ RFID Tag Detected: ${rfid}`;

      // Set the RFID value
      document.getElementById("vehicleRFID").value = rfid;

      // Enable submit button
      document.getElementById("submitVehicleBtn").disabled = false;

      // Hide start scan button
      const startScanBtn = document.getElementById("startRFIDScanBtn");
      if (startScanBtn) {
        startScanBtn.style.display = "none";
      }

      // Close SSE connection
      if (vehicleEventSource) {
        vehicleEventSource.close();
        vehicleEventSource = null;
      }
    }
  });

  vehicleEventSource.addEventListener("rfid_status", (e) => {
    console.log("RFID Status:", e.data);
    scanText.textContent = e.data;
  });

  vehicleEventSource.onerror = (error) => {
    console.error("RFID SSE Error:", error);
    showRFIDScanError("Connection to scanner lost. Please try again.");
  };
}

function showRFIDScanError(message) {
  const scanStatus = document.getElementById("rfidScanStatus");
  const scanText = scanStatus.querySelector(".scan-text");
  scanStatus.className = "scan-status error";
  scanText.textContent = "❌ " + message;
  document.getElementById("submitVehicleBtn").disabled = true;

  const startScanBtn = document.getElementById("startRFIDScanBtn");
  if (startScanBtn) {
    startScanBtn.style.display = "block";
    startScanBtn.disabled = false;
    startScanBtn.textContent = "Start Scan";
  }
}

// ============================================================================
// VEHICLE FORM SUBMISSION
// ============================================================================

// Replace the submitVehicle and loadVehicleList functions in script.js

async function submitVehicle(event) {
  event.preventDefault();

  // IMPORTANT: Close any active SSE connections before saving
  if (vehicleEventSource) {
    console.log("Closing SSE connection before save...");
    vehicleEventSource.close();
    vehicleEventSource = null;
    // Wait a moment for connection to fully close
    await new Promise((resolve) => setTimeout(resolve, 300));
  }

  const rfid = document.getElementById("vehicleRFID").value;
  const plateNo = document.getElementById("plateNo").value;
  const type = document.getElementById("vehicleType").value;
  const owner = document.getElementById("vehicleOwner").value;
  const role = document.getElementById("vehicleRole").value;
  const year = document.getElementById("vehicleYear").value;
  const section = document.getElementById("vehicleSection").value;
  const course = document.getElementById("vehicleCourse").value;

  if (!rfid && !editingVehicleRFID) {
    showNotification(
      "Missing Information",
      "RFID is required. Please scan an RFID tag first.",
      "error"
    );
    return;
  }

  const vehicleData = {
    rfid: editingVehicleRFID || rfid,
    plateNo: plateNo,
    type: type,
    owner: owner,
    role: role,
    year: year,
    section: section,
    course: course,
  };

  console.log("Submitting vehicle data:", vehicleData);

  try {
    const response = await fetch(`${BASE_URL}/vehicle/save`, {
      method: "POST",
      headers: {
        "Content-Type": "application/json",
      },
      body: JSON.stringify(vehicleData),
    });

    console.log("Response status:", response.status);

    if (!response.ok) {
      const errorText = await response.text();
      console.error("Error response:", errorText);
      throw new Error("Failed to save vehicle data: " + errorText);
    }

    const result = await response.json();
    console.log("Save result:", result);

    closeVehicleModal();

    // Wait longer before reloading to ensure ESP32 has fully processed
    await new Promise((resolve) => setTimeout(resolve, 1000));

    console.log("Reloading vehicle list...");
    await loadVehicleList(); // Now uses retry logic

    if (editingVehicleRFID) {
      showNotification(
        "Updated Successfully",
        `Vehicle information has been updated.\n\nRFID: ${vehicleData.rfid}\nPlate: ${plateNo}\nOwner: ${owner}`,
        "success"
      );
    } else {
      showNotification(
        "Registered Successfully",
        `New vehicle has been registered.\n\nRFID: ${vehicleData.rfid}\nPlate: ${plateNo}\nOwner: ${owner}`,
        "success"
      );
    }
  } catch (error) {
    console.error("Error saving vehicle:", error);
    showNotification(
      "Save Failed",
      "Unable to save vehicle data: " + error.message,
      "error"
    );
  }
}

async function loadVehicleList(retryCount = 0) {
  const maxRetries = 3;
  const retryDelay = 1000; // 1 second

  try {
    console.log(`Loading vehicle list (attempt ${retryCount + 1})...`);

    // Add timeout to the fetch request
    const controller = new AbortController();
    const timeoutId = setTimeout(() => controller.abort(), 5000); // 5 second timeout

    const response = await fetch(`${BASE_URL}/vehicle/list`, {
      signal: controller.signal,
    });

    clearTimeout(timeoutId);

    if (!response.ok) {
      throw new Error(`HTTP error! status: ${response.status}`);
    }

    const vehicles = await response.json();
    console.log("Loaded vehicles:", vehicles);
    console.log("Number of vehicles:", vehicles.length);

    const tableBody = document.querySelector(".vehicle-table tbody");
    if (!tableBody) {
      console.error("Table body not found!");
      return;
    }

    tableBody.innerHTML = "";

    if (vehicles.length === 0) {
      const emptyRow = document.createElement("tr");
      emptyRow.innerHTML = `
        <td colspan="9" style="text-align: center; color: #999; padding: 40px;">
          No vehicles registered yet. Click the + button to add one.
        </td>
      `;
      tableBody.appendChild(emptyRow);
    } else {
      vehicles.forEach((vehicle) => {
        const row = document.createElement("tr");
        row.innerHTML = `
          <td>${vehicle.rfid || "N/A"}</td>
          <td>${vehicle.plateNo || "N/A"}</td>
          <td>${vehicle.type || "N/A"}</td>
          <td>${vehicle.owner || "N/A"}</td>
          <td>${vehicle.role || "N/A"}</td>
          <td>${vehicle.year || "N/A"}</td>
          <td>${vehicle.section || "N/A"}</td>
          <td>${vehicle.course || "N/A"}</td>
          <td class="table-action-btn">
            <button class="edit-vehicle-btn" onclick="editVehicle('${
              vehicle.rfid
            }', '${vehicle.plateNo}', '${vehicle.type}', '${vehicle.owner}', '${
          vehicle.role
        }', '${vehicle.year}', '${vehicle.section}', '${vehicle.course}')">
              <svg width="30" height="30" viewBox="0 0 30 30" fill="none" xmlns="http://www.w3.org/2000/svg">
                <g clip-path="url(#clip0_44_202)">
                  <path d="M15 30C23.2843 30 30 23.2843 30 15C30 6.71573 23.2843 0 15 0C6.71573 0 0 6.71573 0 15C0 23.2843 6.71573 30 15 30Z" fill="#26A1F4"/>
                  <path d="M20.2236 13.9183L20.2154 13.9101L16.081 9.77283C16.081 9.77283 11.042 14.8119 8.61385 17.2904C8.3115 17.5986 8.0824 18.0275 7.94646 18.4424C7.54803 19.6605 7.21814 20.9033 6.85135 22.1338C6.75291 22.4636 6.77283 22.7543 7.03123 23.0004C7.27498 23.2347 7.5492 23.2429 7.8656 23.148C9.03748 22.7965 10.217 22.4619 11.3941 22.125C11.9994 21.9495 12.5482 21.6184 12.9855 21.1646C15.2988 18.8373 20.2236 13.9183 20.2236 13.9183Z" fill="white"/>
                  <path d="M22.6371 8.77792L21.2232 7.36406C20.8606 7.00178 20.3691 6.79828 19.8565 6.79828C19.344 6.79828 18.8524 7.00178 18.4898 7.36406L16.8873 8.96484L21.0351 13.1139L22.6377 11.5113C22.9999 11.1487 23.2033 10.6571 23.2031 10.1445C23.203 9.63196 22.9994 9.14042 22.6371 8.77792Z" fill="white"/>
                </g>
              </svg>
            </button>
            <button class="delete-vehicle-btn" onclick="deleteVehicle('${
              vehicle.rfid
            }')">
              <svg width="30" height="30" viewBox="0 0 30 30" fill="none" xmlns="http://www.w3.org/2000/svg">
                <path fill-rule="evenodd" clip-rule="evenodd" d="M15 0C6.72902 0 0 6.72902 0 15C0 23.271 6.72885 30 15 30C23.2712 30 30 23.271 30 15C30 6.72896 23.2711 0 15 0ZM12.0749 6.69897C12.075 6.56966 12.1263 6.44564 12.2177 6.35418C12.3091 6.26273 12.4331 6.21131 12.5624 6.21123H17.4374C17.5668 6.21152 17.6908 6.26309 17.7822 6.35462C17.8736 6.44616 17.925 6.57019 17.9251 6.69955V7.90523H12.0749V6.69897ZM20.0906 23.0911C20.0781 23.2813 19.9934 23.4595 19.8539 23.5892C19.7143 23.719 19.5304 23.7905 19.3399 23.7891H10.6035C10.4131 23.7886 10.2298 23.716 10.0906 23.586C9.95144 23.456 9.86659 23.2781 9.85313 23.0881L9.10588 12.1523H20.888L20.0906 23.0911ZM22.0324 11.1621H7.96758V10.029C7.96783 9.7285 8.08729 9.44041 8.29974 9.22794C8.51218 9.01547 8.80026 8.89598 9.10072 8.8957L20.8991 8.89535C21.1996 8.89577 21.4876 9.01536 21.7001 9.22789C21.9125 9.44042 22.0319 9.72854 22.0322 10.029L22.0324 11.1621ZM12.7354 21.1821V14.2383C12.7354 14.107 12.7876 13.9811 12.8805 13.8882C12.9734 13.7954 13.0993 13.7433 13.2306 13.7433C13.362 13.7434 13.4879 13.7956 13.5807 13.8885C13.6735 13.9814 13.7256 14.1073 13.7256 14.2386V21.1821C13.7266 21.2478 13.7145 21.313 13.6901 21.3739C13.6657 21.4349 13.6294 21.4904 13.5833 21.5372C13.5372 21.584 13.4823 21.6211 13.4217 21.6465C13.3611 21.6718 13.2961 21.6849 13.2305 21.6849C13.1648 21.6849 13.0998 21.6718 13.0392 21.6465C12.9786 21.6211 12.9237 21.584 12.8776 21.5372C12.8316 21.4904 12.7953 21.4349 12.7708 21.3739C12.7464 21.313 12.7343 21.2478 12.7354 21.1821ZM16.2681 21.1821V14.2383C16.2701 14.1083 16.3232 13.9843 16.4159 13.8931C16.5086 13.8019 16.6334 13.7509 16.7634 13.7509C16.8934 13.7509 17.0182 13.8021 17.1108 13.8934C17.2034 13.9846 17.2564 14.1086 17.2583 14.2386V21.1824C17.2598 21.2483 17.248 21.3139 17.2238 21.3752C17.1995 21.4365 17.1633 21.4924 17.1172 21.5395C17.0711 21.5867 17.016 21.6241 16.9552 21.6497C16.8944 21.6752 16.8291 21.6884 16.7632 21.6884C16.6973 21.6884 16.632 21.6752 16.5712 21.6497C16.5104 21.6241 16.4553 21.5867 16.4092 21.5395C16.3631 21.4924 16.3269 21.4365 16.3026 21.3752C16.2784 21.3139 16.2667 21.248 16.2681 21.1821Z" fill="#FC0005"/>
                <path fill-rule="evenodd" clip-rule="evenodd" d="M20.0906 23.0911C20.0781 23.2813 19.9934 23.4595 19.8539 23.5892C19.7143 23.719 19.5304 23.7905 19.3399 23.7891H10.6035C10.4131 23.7886 10.2298 23.716 10.0906 23.586C9.95144 23.456 9.86659 23.2781 9.85313 23.0881L9.10588 12.1523H20.888L20.0906 23.0911ZM12.7354 14.2383V21.1821C12.7343 21.2478 12.7464 21.313 12.7708 21.3739C12.7953 21.4349 12.8316 21.4904 12.8776 21.5372C12.9237 21.584 12.9786 21.6211 13.0392 21.6465C13.0998 21.6718 13.1648 21.6849 13.2305 21.6849C13.2961 21.6849 13.3611 21.6718 13.4217 21.6465C13.4823 21.6211 13.5372 21.584 13.5833 21.5372C13.6294 21.4904 13.6657 21.4349 13.6901 21.3739C13.7145 21.313 13.7266 21.2478 13.7256 21.1821V14.2386C13.7256 14.1073 13.6735 13.9814 13.5807 13.8885C13.4879 13.7956 13.362 13.7434 13.2306 13.7433C13.0993 13.7433 12.9734 13.7954 12.8805 13.8882C12.7876 13.9811 12.7354 14.107 12.7354 14.2383ZM16.2681 14.2383V21.1821C16.2667 21.248 16.2784 21.3139 16.3026 21.3752C16.3269 21.4365 16.3631 21.4924 16.4092 21.5395C16.4553 21.5867 16.5104 21.6241 16.5712 21.6497C16.632 21.6752 16.6973 21.6884 16.7632 21.6884C16.8291 21.6884 16.8944 21.6752 16.9552 21.6497C17.016 21.6241 17.0711 21.5867 17.1172 21.5395C17.1633 21.4924 17.1995 21.4365 17.2238 21.3752C17.248 21.3139 17.2598 21.2483 17.2583 21.1824V14.2386C17.2564 14.1086 17.2034 13.9846 17.1108 13.8934C17.0182 13.8021 16.8934 13.7509 16.7634 13.7509C16.6334 13.7509 16.5086 13.8019 16.4159 13.8931C16.3232 13.9843 16.2701 14.1083 16.2681 14.2383Z" fill="white"/>
              </svg>
            </button>
          </td>`;
        tableBody.appendChild(row);
      });
    }
  } catch (error) {
    console.error(
      `Error loading vehicle list (attempt ${retryCount + 1}):`,
      error
    );

    // Retry logic
    if (retryCount < maxRetries) {
      console.log(`Retrying in ${retryDelay}ms...`);
      await new Promise((resolve) => setTimeout(resolve, retryDelay));
      return loadVehicleList(retryCount + 1);
    } else {
      // Only show error after all retries failed
      showNotification(
        "Load Failed",
        "Failed to load vehicle list after multiple attempts. Please refresh the page.",
        "error"
      );
    }
  }
}
// ============================================================================
// EDIT VEHICLE FUNCTION
// ============================================================================

function editVehicle(rfid, plateNo, type, owner, role, year, section, course) {
  editingVehicleRFID = rfid;

  const modal = document.getElementById("vehicleModal");
  modal.classList.add("active");
  document.body.style.overflow = "hidden";

  // Change modal title
  const modalTitle = modal.querySelector(".modal-header h2");
  modalTitle.innerHTML = `
    <!-- Edit icon SVG -->
    Edit Vehicle Information
  `;

  // Fill form with current data
  document.getElementById("vehicleRFID").value = rfid;
  document.getElementById("plateNo").value = plateNo;
  document.getElementById("vehicleType").value = type;
  document.getElementById("vehicleOwner").value = owner;
  document.getElementById("vehicleRole").value = role;
  document.getElementById("vehicleYear").value = year;
  document.getElementById("vehicleSection").value = section;
  document.getElementById("vehicleCourse").value = course;

  // Hide RFID scanner section
  const scannerSection = document.querySelector(
    "#vehicleModal .form-group:has(#rfidScanStatus)"
  );
  if (scannerSection) {
    scannerSection.style.display = "none";
  }

  // Enable submit button
  document.getElementById("submitVehicleBtn").disabled = false;
  document.getElementById("submitVehicleBtn").textContent =
    "Update Information";
}

// ============================================================================
// DELETE VEHICLE FUNCTIONS
// ============================================================================

async function deleteVehicle(rfid) {
  const confirmed = await showConfirmation(
    "Delete Vehicle",
    `Are you sure you want to delete vehicle with RFID ${rfid}?\n\nThis action cannot be undone.`
  );

  if (!confirmed) {
    return;
  }

  try {
    console.log("Deleting vehicle RFID:", rfid);
    const response = await fetch(`${BASE_URL}/vehicle/delete?rfid=${rfid}`);

    if (response.ok) {
      showNotification(
        "Deleted Successfully",
        `Vehicle with RFID ${rfid} has been removed from the system.`,
        "success"
      );
      await loadVehicleList();
    } else {
      throw new Error("Failed to delete vehicle");
    }
  } catch (error) {
    console.error("Error deleting vehicle:", error);
    showNotification(
      "Delete Failed",
      "Unable to delete vehicle. Please check your connection and try again.",
      "error"
    );
  }
}

async function deleteAllVehicles() {
  const confirmed = await showConfirmation(
    "⚠️ Delete All Vehicles",
    "This will permanently delete ALL vehicles from the system.\n\nThis action CANNOT be undone.\n\nAre you absolutely sure?"
  );

  if (!confirmed) {
    return;
  }

  const doubleCheck = await showConfirmation(
    "Final Confirmation",
    "Please confirm one more time.\n\nDelete ALL vehicles?"
  );

  if (!doubleCheck) {
    return;
  }

  try {
    console.log("Deleting all vehicles...");
    const response = await fetch(`${BASE_URL}/vehicle/deleteall`);

    if (response.ok) {
      const result = await response.text();
      showNotification(
        "All Deleted",
        "All vehicles have been successfully removed from the system.",
        "success"
      );
      await loadVehicleList();
    } else {
      throw new Error("Failed to delete all vehicles");
    }
  } catch (error) {
    console.error("Error deleting all vehicles:", error);
    showNotification(
      "Delete Failed",
      "Unable to delete all vehicles. Please check your connection and try again.",
      "error"
    );
  }
}

// Close modals when clicking outside
document.addEventListener("click", function (event) {
  const notificationModal = document.getElementById("notificationModal");
  const confirmationModal = document.getElementById("confirmationModal");

  if (event.target === notificationModal) {
    closeNotification();
  }

  if (event.target === confirmationModal) {
    closeConfirmation(false);
  }
});
