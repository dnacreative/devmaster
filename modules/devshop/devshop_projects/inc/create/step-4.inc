<?php

/**********
 * STEP 4
 * Project Settings
 *********/

/**
 * STEP 4: Form
 */
function devshop_project_create_step_sites($form, &$form_state) {
  $project = $form_state['project'];
  $profiles = array();
  $available_profiles = array();
  $completed = TRUE;
  $task_failed = FALSE;

  $form['nid'] = array(
    '#type' => 'value',
    '#value' => $project->nid,
  );

  // Display the platforms
  $rows = array();
  $header = array(
    t('Name'),
    t('Branch'),
    t('Version'),
    t('Install Profiles'),
    t('Status')
  );
  $all_tasks_queued = TRUE;
  $all_tasks_succeeded = TRUE;
  foreach ($project->environments as $name => $environment) {

    // Get platform and latest verify task.
    $platform_nid = $environment->platform;
    $platform = node_load($platform_nid);
    $tasks = devshop_get_tasks($environment, 'verify', 'type');
    $task = current($tasks);

    if (empty($task)) {
      $row = array(
        $environment->name,
        $environment->git_ref,
        t('Error: Platform node was not saved. Unable to continue.'),
        '', '',
      );
      $rows[] = $row;
      continue;
    }

    // Build a table.
    $row = array();
    $row['name'] = $name;
    $row['branch'] = $environment->git_ref;
    $row['version'] = t('...');

    // If platform verified successfully:
    if ($task->task_status == HOSTING_TASK_SUCCESS || $task->task_status == HOSTING_TASK_WARNING) {

      // It's not really ready until we get a version.
      if (empty($platform->release->version)) {
        $completed = FALSE;
        $row['version'] = '...';
        $row['profiles'] = '...';
      }
      else {
        $row['version'] = $platform->release->version;
      }

      // Collect install profiles
      $profiles_shortnames = hosting_get_profiles($platform->nid, 'short_name');

      if (is_array($profiles_shortnames) && !empty($profiles_shortnames)) {
        $profiles[$name] = array_combine($profiles_shortnames, (array) hosting_get_profiles($platform->nid));
        $row['profiles'] = implode(', ', $profiles[$name]);
      }
      else {
        $profiles[$name] = array();
      }

      // If no tasks have failed, save available profiles
      if ($all_tasks_succeeded) {
        if (empty($available_profiles)) {
          $available_profiles = $profiles[$name];
        }
        else {
          $available_profiles = array_intersect_key($available_profiles, $profiles[$name]);
        }
      }
    }
    // If platform verification failed:
    elseif ($task->task_status == HOSTING_TASK_ERROR) {
      $completed = TRUE;
      $all_tasks_succeeded = FALSE;
      $available_profiles = array();

      $error = db_query("SELECT message FROM {hosting_task_log} WHERE nid = :nid AND type = :type ORDER BY vid, lid", array(':nid' => $task->nid, ':type' => 'error'))->fetchField();

      $row['version'] = array(
        'data' => '<div class="alert alert-danger small">' . $error . '</div> ',
        'colspan' => 2,
      );
    }
    // If platform is still processing:
    elseif ($task->task_status == HOSTING_TASK_PROCESSING || $task->task_status == HOSTING_TASK_QUEUED) {
      $completed = FALSE;
      $row['version'] = "<span id='version-{$platform->nid}'>...</span>";
      $row['profiles'] = "<span id='profiles-{$platform->nid}'>...</span>";
    }

    // If a single task is not queued, $all_tasks_queued == FALSE
    if ($task->task_status != HOSTING_TASK_QUEUED) {
      $all_tasks_queued = FALSE;
    }

    // If a single task is not queued, $all_tasks_queued == FALSE
    if ($task->task_status == HOSTING_TASK_ERROR) {
      $task_failed = TRUE;
    }

    // Add hosting task status.
    $status = _hosting_parse_error_code($task->task_status);
    if ($status == 'Queued') {
      $status = '...';
    }
    $row['status'] = l("<span id='status-{$platform->nid}'>{$status}</span>", "node/{$task->nid}", array('html' => TRUE));

    // Store rows for display
    $rows[] = $row;
  } // end foreach platform

  // Output our table.
  $form['platforms'] = array(
    '#markup' => theme('table', array('header' => $header, 'rows' => $rows, 'attributes' => array('class' => array('table')))),
  );

  // Not completed means show all tasks are not completed (or errored)
  if (!$completed) {
    $form['#no_finish'] = TRUE;
    $note = '<p class="wait">' . t('Please wait while we clone your repo and verify your drupal code.') . '</p>';

    $form['help'] = array(
      '#markup' => $note,
    );

    // Add code to reload the page when complete.
    devshop_form_reloader($form, 'platform');
    return $form;
  }
  elseif ($task_failed) {
    $project->no_finish = TRUE;
    $form['error'] = array(
      '#prefix' => '<p class="error">',
      '#markup' => t('Something went wrong when trying to clone your codebase.'),
      '#suffix' => '</p>',
    );
    $form['retry'] = array(
      '#type' => 'submit',
      '#value' => t('Retry'),
      '#prefix' => '<div class="text-center">',
      '#suffix' => '</div>',
    );
    $form['#no_finish'] = TRUE;
  }

  // If no available profiles:
  elseif (count($available_profiles) == 0) {
    $project->no_finish = TRUE;
    $note = '<p class="error">' . t('No common profile was found in all of your branches.  Please check your source code and try again.  You must !link to change what environment tracks each branch.', array('!link' => l(t('Go Back'), 'projects#/add/environments'))) . '</p>';

    $form['error'] = array(
      '#markup' => $note,
    );
    $form['#no_finish'] = TRUE;
  }
  else {
    $project->no_finish = FALSE;

    // Install Profile
    // Sensible default?
    // Lets go with standard for now... we can update later.
    if (isset($available_profiles['standard'])) {
      $default_profile = 'standard';
    }
    // If 'drupal' profile exists, it is likely drupal6!
    elseif (isset($available_profiles['drupal'])) {
      $default_profile = 'drupal';
    }

    $form['install_profile'] = array(
      '#type' => 'radios',
      '#options' => $available_profiles,
      '#title' => t('Project Install Profile'),
      '#required' => 1,
      '#description' => t('All sites in your project must use the same installation profile, and it must exist in all branches.  Choose the installation profile for this project.'),
      '#default_value' => $default_profile,
    );
  }
  return $form;
}

/**
 * STEP 4: Validate
 */
function devshop_project_create_step_sites_validate(&$from, &$form_state) {
  if ($form_state['triggering_element']['#value'] == t('Retry')) {
    foreach ($form_state['project']->environments as $name => $environment) {
      $task = current($environment->tasks['verify']);
      if (isset($task->nid)) {
        hosting_task_retry($task->nid);
      }
    }
  }
  elseif (empty($form_state['values']['install_profile'])) {
    form_set_error('install_profile', t('You must choose an install profile. Please wait for all environments to verify.'));
  }
}
