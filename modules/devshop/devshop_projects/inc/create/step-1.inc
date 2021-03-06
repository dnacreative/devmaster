<?php

/**********
 * STEP 1
 * Git URL
 **********/

/**
 * STEP 1: Form
 *
 * Just get the Git URL.  If we can't access the code, all else should fail.
 */
function devshop_project_create_step_git($form, $form_state) {
  $project = &$form_state['project'];

  drupal_add_js(drupal_get_path('module', 'devshop_projects') . '/inc/create/create.js');

  if ($project->verify_error) {
    $form['note'] = array(
      '#value' => '<div class="messages error">' . $project->verify_error . '</div>',
      '#type' => 'markup',
    );

    // Display some help for certain errors
    if ($project->verify_error == '[DEVSHOP] Error retrieving remote information: Host key verification failed. fatal: The remote end hung up unexpectedly') {
      $form['note']['#value'] .= '<div class="messages warning">' . t('You might have to authorize the host your are trying to connect to first.  Add "StrictHostKeyChecking no" to your ~/.ssh/config file to avoid this for all domains.') . '</div>';
    }
  }

  if (empty($project->name)) {
    $form['title'] = array(
      '#type' => 'textfield',
      '#title' => t('Project Code Name'),
      '#required' => TRUE,
      '#suffix' => '<p>' . t('Choose a unique name for your project.') . '</p>',
      '#size' => 40,
      '#maxlength' => 255,
    );
  }
  else {
    $form['title'] = array(
      '#type' => 'value',
      '#value' => $project->name,
    );
    $form['title_display'] = array(
      '#type' => 'item',
      '#title' => t('Project Code Name'),
      '#markup' => $project->name,
    );
  }

  $form['git_url'] = array(
    '#type' => 'textfield',
    '#required' => 1,
    '#title' => t('Git URL'),
    '#suffix' => '<p>' . t('Enter the Git URL for your project. You should use the "ssh" url when possible. An entire Drupal codebase must be present in the root, unless a composer.json file is present.') . '</p>',
    '#default_value' => $project->git_url,
  );
//
//  // Project code path.
//  $form['code_path'] = array(
//    '#type' => variable_get('devshop_projects_allow_custom_code_path', FALSE) ? 'textfield' : 'value',
//    '#title' => t('Code path'),
//    '#description' => t('The absolute path on the filesystem that will be used to create all platforms within this project. There must not be a file or directory at this path.'),
//    '#required' => variable_get('devshop_projects_allow_custom_code_path', FALSE),
//    '#size' => 40,
//    '#default_value' => $project->code_path,
//    '#maxlength' => 255,
//    '#attributes' => array(
//      'data-base_path' => variable_get('devshop_project_base_path', '/var/aegir/projects'),
//    ),
//  );
//
//  // Project base url
//  $form['base_url'] = array(
//    '#type' => variable_get('devshop_projects_allow_custom_base_url', FALSE) ? 'textfield' : 'value',
//    '#title' => t('Base URL'),
//    '#description' => t('All sites will be under a subdomain of this domain.'),
//    '#required' => variable_get('devshop_projects_allow_custom_base_url', FALSE),
//    '#size' => 40,
//    '#default_value' => $project->base_url,
//    '#maxlength' => 255,
//    '#attributes' => array(
//      'data-base_url' => devshop_projects_url($project->name, ''),
//    ),
//  );

  // Display helpful tips for connecting.
  $pubkey = variable_get('devshop_public_key', '');

  // If we don't yet have the server's public key saved as a variable...
  if (empty($pubkey)) {
    $output = t("For convenience, the server's SSH public key will be displayed here, once you run the following command on your server:");
    $command = 'drush @hostmaster vset devshop_public_key "$(cat ~/.ssh/id_rsa.pub)" --yes';
    $output .= "<div class='command'><input size='160' value='$command' onclick='this.select()' /></div>";
  }
  else {
    // @TODO: Make this Translatable
    $output = <<<HTML
  <div class="empty-message">If you haven't granted this server access to your Git repository, you should do so now using it's public SSH key.</div>
  <textarea id="rsa" onclick='this.select()'>$pubkey</textarea>
HTML;
  }

  // Add info about connecting to Repo
  $form['connect'] = array(
    '#type' => 'fieldset',
    '#title' => t('Repository Access'),
    '#collapsible' => TRUE,
    '#collapsed' => TRUE,
  );
  $form['connect']['public_key'] = array(
    '#markup' => $output,
  );
  return $form;
}

/**
 * STEP 1: Validate
 */
function devshop_project_create_step_git_validate(&$form, &$form_state) {
  $project = &$form_state['project'];

  if (empty($project->nid)) {

    // PROJECT NAME
    // No spaces or special characters allowed.
    $project_name = strtolower(trim($form_state['values']['title'])); // domain names are case-insensitive

    // Set the value to our trimmed and lowercased proejct name.
    form_set_value($form['title'], $project_name, $form_state);

    // Check for bad characters.
    if (!ctype_alnum($project_name)) {
      form_set_error('title', t("You have not specified a valid project codename.  Only numbers and letters are allowed."));
    }

    // Check for duplicate project name here.
    $node = hosting_context_load($project_name);
    if ($node->nid != $project->nid) {
      form_set_error('title', t('That name is in use.  Please try again.'));
    }
  }
//
//  // CODE PATH & BASE URL
//  // Code path and base url must be unique
//  if (db_query('SELECT n.nid FROM {hosting_devshop_project} d LEFT JOIN {node} n ON d.nid = n.nid WHERE status = :status AND code_path = :code_path;', array(':status' => 1, ':code_path' => $form_state['values']['code_path']))->fetchField()) {
//    form_set_error('code_path', t('Another project already has this code path.  Code path must be unique.'));
//  }
//  if (db_query('SELECT n.nid FROM {hosting_devshop_project} d LEFT JOIN {node} n ON d.nid = n.nid WHERE status = :status AND base_url = :base_url;', array(':status' => 1, ':base_url' => $form_state['values']['base_url']))->fetchField()) {
//    form_set_error('base_url', t('Another project already has this base url.  Base URL must be unique.'));
//  }
//
//  // If custom code paths are not allowed, set the value here using the project name.
//  if (!variable_get('devshop_projects_allow_custom_code_path', FALSE)) {
//    form_set_value($form['code_path'], variable_get('devshop_project_base_path', '/var/aegir/projects') . '/' . $project_name, $form_state);
//  }
//
//  // If custom base URLs are not allowed, set the value here using the project name.
//  if (!variable_get('devshop_projects_allow_custom_base_url', FALSE)) {
//    form_set_value($form['base_url'], devshop_projects_url($project_name), $form_state);
//  }
}

/**
 * STEP 1: Submit
 */
function devshop_project_create_step_git_submit(&$from, &$form_state) {
  global $user;

  $project = &$form_state['project'];

  // If the project already exists, this means the git url has changed...
  if ($project->nid) {
    // Change the git url and save the node.  Verification will run again.
    $node = node_load($project->nid);
    $node->project->git_url = $form_state['values']['git_url'];
    $node->project->code_path = $form_state['values']['code_path'];
    $node->project->base_url = $form_state['values']['base_url'];
    node_save($node);
  }
  // Create new project node
  elseif (empty($project->nid)) {
    // Create the project node now.  We will update it with the chosen path.
    // This is so we can check for branches and save the hosting_context as soon
    // as possible.
    $node = new stdClass;
    $node->title = $form_state['values']['title'];

    $node->type = 'project';
    $node->status = 0;
    $node->uid = $user->uid;
    $node->name = $user->name;

    // Create project object
    $project->name = $form_state['values']['title'];
    $project->git_url = $form_state['values']['git_url'];
    $project->code_path = $form_state['values']['code_path'];
    $project->base_url = $form_state['values']['base_url'];

    // @TODO: We will clone the code for step 2 and look for drupal.
    $project->drupal_path = variable_get('devshop_projects_default_drupal_path', '');

    // Attach to node
    $node->project = $project;

    // Save project node, triggering verification.
    if ($node = node_submit($node)) {
      node_save($node);
    }

    // Save NID to ctools object cache.
    if ($node->nid) {
      $project->nid = $node->nid;
    }
  }

  // Remove default "task" messages.
  drupal_get_messages();
}
