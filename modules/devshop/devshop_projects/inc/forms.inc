<?php
/**
 * @file devshop_project.form.inc DevShop Project forms related hooks and
 * support function include file.
 */

/**
 * Implementation of hook_form_FORM_ID_alter().
 *
 * The Project Node Form.
 */
function devshop_projects_form_project_node_form_alter(&$form, &$form_state, $form_id) {
  $node = $form['#node'];
  $project = $node->project;

  // Save last project data
  $form['old'] = array(
    '#value' => $node,
    '#type' => 'value',
  );

  // Project Settings
  // Every value under $form['project'] gets serialized and saved into a project's "data" column.
  $form['project'] = array(
    '#tree' => TRUE,
    '#type' => 'fieldset',
  );

  // Hidden fields that can't change.
  $form['title'] = array(
    '#type' => 'textfield',
    '#title' => t('Project Code Name'),
    '#required' => TRUE,
    '#description' => t('Choose a unique name for your project.'),
    '#size' => 40,
    '#default_value' => $node->title,
    '#maxlength' => 255,
  );
  $form['project']['git_url'] = array(
    '#type' => 'value',
    '#value' => $project->git_url,
  );
  $form['project']['code_path'] = array(
    '#type' => 'textfield',
    '#title' => t('Code path'),
    '#description' => t('The absolute path on the filesystem that will be used to create the platform in the directory specified above.'),
    '#required' => TRUE,
    '#size' => 40,
    '#default_value' => $project->code_path,
    '#maxlength' => 255,
    '#weight' => 2,
  );
  $form['project']['drupal_path'] = array(
    '#type' => 'textfield',
    '#title' => t('Path to Drupal'),
    '#description' => t('Enter the relative path to the index.php file in your repository. Leave blank if index.php is in the root.'),
    '#size' => 40,
    '#default_value' => $project->drupal_path,
    '#maxlength' => 255,
    '#weight' => 3,
  );
  $form['project']['base_url'] = array(
    '#type' => 'textfield',
    '#title' => t('Base URL'),
    '#description' => t('The base domain name for this project. All environments will get a subdomain under this one.  <em>This will only affect new environments.</em>'),
    '#required' => TRUE,
    '#size' => 40,
    '#default_value' => $project->base_url,
    '#maxlength' => 255,
    '#weight' => 4,
    '#access' => variable_get('devshop_projects_allow_custom_base_url', FALSE),
  );

  // Don't allow editing
  if ($node->nid) {

    // Title
    $form['title']['#value'] = $form['title']['#default_value'];
    $form['title']['#type'] = 'value';

  }

  // Prevent editing code path unless allowed.
  if (!variable_get('devshop_projects_allow_custom_code_path', FALSE)) {
    $form['project']['code_path']['#type'] = 'value';
    $form['project']['code_path']['#value'] = $form['project']['code_path']['#default_value'];
  }

  // Project Settings
  $form['project']['settings'] = array(
    '#weight' => -10,
  );

  // Project Settings Vertical Tabs
  $form['project_settings'] = array(
    '#type' => 'vertical_tabs',
    '#weight' => -11,
  );

  // Save git branches and tags
  $form['project']['settings']['git']['branches'] = array(
    '#type' => 'value',
    '#value' => isset($project->settings->git['branches']) ? $project->settings->git['branches'] : NULL,
  );
  $form['project']['settings']['git']['tags'] = array(
    '#type' => 'value',
    '#value' => isset($project->settings->git['tags']) ? $project->settings->git['tags'] : NULL,
  );

  // Live Environment settings.
  $form['project']['settings']['live'] = array(
    '#type' => 'fieldset',
    '#collapsible' => TRUE,
    '#collapsed' => arg(1) != $project->nid,
    '#title' => t('Domain Name Settings'),
    '#group' => 'project_settings',
  );

  // Live Environment
  $environments = array_keys($project->environments);
  if (empty($environments)) {
    $environments_options = array();
  }
  else {
    $environments_options = array_combine($environments, $environments);
    $environments_options[''] = t('No live environment');
  }

  $form['project']['settings']['live']['live_environment'] = array(
    '#type' => 'select',
    '#title' => t('Live environment'),
    '#description' => t('The environment for the live website.  This locks the live environment from overwrites and disabling, and allows syncing to other environments.'),
    '#options' => $environments_options,
    '#default_value' => isset($project->settings->live) ? $project->settings->live['live_environment'] : '',
  );

  // Live Domain
  $form['project']['settings']['live']['live_domain'] = array(
    '#type' => 'textfield',
    '#title' => t('Live Domain'),
    '#description' => t('The live domain for this project. Used only for links and when creating subdomain aliases for other environments. <em>You must still add the Domain to your live environment manually.</em>'),
    '#size' => 40,
    '#default_value' => isset($project->settings->live) ? $node->project->settings->live['live_domain'] : '',
  );

  // Use aliases
  $form['project']['settings']['live']['environment_aliases'] = array(
    '#type' => 'checkbox',
    '#title' => t('For new environments, create subdomains under Live Domain.'),
    '#description' => t('When new environments are created, automatically add a domain name such as http://ENVIRONMENT.LIVEDOMAIN.com. <em>Does not affect existing environments. Does not remove domains when disabled.</em>'),
    '#default_value' => isset($project->settings->live) ? $project->settings->live['environment_aliases'] : FALSE,
  );

  // Pull Code Method
  $form['project']['settings']['deploy'] = array(
    '#type' => 'fieldset',
    '#collapsible' => TRUE,
    '#collapsed' => arg(1) != $project->nid,
    '#title' => t('Deployment Automation'),
    '#description' => t('Configure how code is delivered to the servers. Post Deploy hooks are configured per environment.'),
    '#weight' => -10,
    '#group' => 'project_settings',
  );

  $form['project']['settings']['deploy']['method'] = array(
    '#title' => 'Deploy Code Method',
    '#type' => 'radios',
    '#description' => t('Choose the method used to deploy code to the server.'),
    '#default_value' => isset($project->settings->deploy['method']) ? $project->settings->deploy['method'] : 'webhook',
  );

  // Commit Webhook
  $form['project']['settings']['deploy']['method']['#options']['webhook'] = t('Immediate Deployment');
  $form['project']['settings']['deploy']['method']['#options']['webhook'] .= '<div class="description">' . t('Recommended. Deploy code as it is delivered to your repository.') . ' <em>' . t('Requires setting up a webhook with your git repository host.') . '</em>' . '</div>';

  // Queue
  $queues = hosting_get_queues();
  if ($queues['deploy']['enabled']) {
    $form['project']['settings']['deploy']['method']['#options']['queue'] = t('Queued Deployment');

    $t = array();
    $t['@freq'] = format_interval($queues['deploy']['frequency'], 1);

    $form['project']['settings']['deploy']['method']['#options']['queue'] .= '<div class="description">';
    $form['project']['settings']['deploy']['method']['#options']['queue'] .= t('Deploy code every @freq. Only runs deploy hooks if code has changed.  Use only if repository webhooks are not available.', $t);


    if (user_access('administer hosting queues')) {
      $form['project']['settings']['deploy']['method']['#options']['queue'] .= ' ' . l(t("Deploy Queue configured to run every @freq.", $t), 'admin/hosting/queues');
    }
    else {
      $form['project']['settings']['deploy']['method']['#options']['queue'] .= ' ' . t("Deploy Queue configured to run every @freq.", $t);

    }

    $form['project']['settings']['deploy']['method']['#options']['queue'] .= '</div>';

  }

  // Manual Pull
  $form['project']['settings']['deploy']['method']['#options']['manual'] = t('Manual Deployment');
  $form['project']['settings']['deploy']['method']['#options']['manual'] .= '<div class="description">' . t('Deploy code to servers manually via devshop or drush.');

  $form['project']['settings']['deploy']['method']['#options']['manual'] .= ' <em>' . t('Not recommended. All environments must be manually updated.') . '</em>' . '</div>';

  // Add link to hosting queues admin if the user can access them.
  if (!$queues['deploy']['enabled'] && user_access('administer hosting queues')) {
    $form['project']['settings']['deploy']['queue_admin'] = array(
      '#value' => t('The !link is disabled. Enable it to allow projects to pull code in the queue.', array(
        '!link' => l(t('Pull Queue'), 'admin/hosting/queues'),
      )),
      '#prefix' => '<p>',
      '#suffix' => '</p>',
    );
  }

  // Load deploy hooks form element.
  $form['project']['settings']['deploy']['default_hooks'] = devshop_environment_deploy_hooks_form($project);

  // Deploy hooks configuration
  $form['project']['settings']['deploy']['default_hooks']['label'] = array(
    '#value' => t('Default Deploy Hooks:'),
    '#prefix' => '<label>',
    '#suffix' => '</label>',
    '#weight' => -100,
  );
  $form['project']['settings']['deploy']['default_hooks']['description'] = array(
    '#value' => t('New environments will run these actions when new code or data is deployed.'),
    '#prefix' => '<p>',
    '#suffix' => '</p>',
    '#weight' => -100,
  );

  $form['project']['settings']['deploy']['default_hooks']['allow_environment_deploy_config'] = array(
    '#type' => 'checkbox',
    '#title' => t('Allow environment-specific deploy hook configuration.'),
    '#default_value' => isset($node->project->settings->deploy) ? $node->project->settings->deploy['allow_environment_deploy_config'] : FALSE,
    '#description' => t('Each environment can be configured to have different deploy hooks. <em>Be sure to check your environments settings if you enable this.</em>' ),
    '#parents' => array(
      'project',
      'settings',
      'deploy',
      'allow_environment_deploy_config',
    ),
  );
  $form['project']['settings']['deploy']['default_hooks']['allow_environment_deploy_hooks_override'] = array(
    '#type' => 'checkbox',
    '#title' => t('Allow users to override hooks when deploying manually.'),
    '#default_value' => isset($node->project->settings->deploy) ? $node->project->settings->deploy['allow_environment_deploy_hooks_override'] : FALSE,
    '#description' => t('Check this box to allow users to override the hooks that are run on manual deployments. If left unchecked, all environments will run the deploy hooks configured above on every deployment.' ),
    '#parents' => array(
      'project',
      'settings',
      'deploy',
      'allow_environment_deploy_hooks_override',
    ),
  );

  // @TODO: is there a better way to save certain values? We lose data without these.
  $form['project']['settings']['deploy']['last_webhook'] = array(
    '#type' => 'value',
    '#value' => isset($node->project->settings->deploy) ? $node->project->settings->deploy['last_webhook'] : NULL,
  );
  $form['project']['settings']['deploy']['last_webhook_status'] = array(
    '#type' => 'value',
    '#value' => isset($node->project->settings->deploy) ? $node->project->settings->deploy['last_webhook_status'] : NULL,
  );
  $form['project']['settings']['deploy']['last_webhook_ip'] = array(
    '#type' => 'value',
    '#value' => isset($node->project->settings->deploy) ? $node->project->settings->deploy['last_webhook_ip'] : NULL,
  );

  //All settings git pull in project page
  $form['project']['settings']['default_environment'] = array(
    '#type' => 'fieldset',
    '#collapsible' => TRUE,
    '#collapsed' => arg(1) != $project->nid,
    '#title' => t('Default Environment Settings'),
    '#description' => t('These settings will be used when creating new environments.'),
    '#group' => 'project_settings',
  );

  // Install Profile
  $available_profiles = array();
  foreach ($project->environments as $name => $environment) {
    $profiles_shortnames = hosting_get_profiles($environment->platform, 'short_name');
    if (is_array($profiles_shortnames) && !empty($profiles_shortnames)) {
      $profiles[$name] = array_combine($profiles_shortnames, (array)hosting_get_profiles($environment->platform));
    } else {
      $profiles[$name] = array();
    }
    if (empty($available_profiles)) {
      $available_profiles = $profiles[$name];
    } else {
      $available_profiles = array_intersect_key($available_profiles, $profiles[$name]);
    }
  }

  $form['project']['settings']['default_environment']['install_profile'] = array(
    '#type' => 'radios',
    '#options' => $available_profiles,
    '#title' => t('Default Install Profile'),
    '#required' => 1,
    '#description' => t('New environments will be created using this install profile. Existing environments will not be affected.'),
    '#default_value' => $node->project->install_profile,
    '#access' => count($available_profiles),
  );

  // HTTP Server select.
  $http_servers = hosting_get_servers('http');
  if (count($http_servers)) {
    $form['project']['settings']['default_environment']['web_server'] = array(
      '#title' => t('Web server'),
      '#type' => 'select',
      '#options' => $http_servers,
      '#default_value' => isset($project->settings->default_environment) ? $project->settings->default_environment['web_server'] : current($http_servers),
    );
  }

  // DB Server select.
  $db_servers = hosting_get_servers('db');
  if (count($db_servers)) {
    $form['project']['settings']['default_environment']['db_server'] = array(
      '#title' => t('Database server'),
      '#type' => 'select',
      '#options' => $db_servers,
      '#default_value' => isset($project->settings->default_environment) ? $project->settings->default_environment['db_server'] : current($db_servers),
    );
  }

  // Solr Server Select
  $solr_servers = hosting_get_servers('solr');
  if (count($solr_servers)) {
    $form['project']['settings']['default_environment']['solr_server'] = array(
      '#title' => t('Solr server'),
      '#type' => 'select',
      '#options' => $solr_servers,
      '#default_value' => $project->settings->default_environment['solr_server'],
    );
  }
  
  // Force the servers for this project's environments
  $form['project']['settings']['default_environment']['force_default_servers'] = array(
    '#title' => t('Force new environments to use these servers.'),
    '#description' => t('When new environments are created, they will use the servers selected above.'),
    '#type' => 'checkbox',
    '#default_value' => $project->settings->default_environment['force_default_servers'],
  );
  return $form;
}

/**
 * Implementation of hook_validate().
 *
 * This function is no longer used since we have a ctools wizard for
 * project creation.
 */
function devshop_projects_validate($node, &$form, &$form_state) {

  // Code path and base url must be unique
  if (db_query('SELECT n.nid FROM {hosting_devshop_project} d LEFT JOIN {node} n ON d.nid = n.nid WHERE status = :status AND code_path = :code_path;', array(':status' => 1, ':code_path' => $form_state['values']['project']['code_path']))->fetchField()) {
    form_set_error('code_path', t('Another project already has this code path.  Code path must be unique.'));
  }
  if (db_query('SELECT n.nid FROM {hosting_devshop_project} d LEFT JOIN {node} n ON d.nid = n.nid WHERE status = :status AND base_url = :base_url;', array(':status' => 1, ':base_url' => $form_state['values']['project']['base_url']))->fetchField()) {
    form_set_error('base_url', t('Another project already has this base url.  Base URL must be unique.'));
  }

  // If custom code paths are not allowed, set the value here using the project name.
  if (!variable_get('devshop_projects_allow_custom_code_path', FALSE)) {
    form_set_value($form['code_path'], variable_get('devshop_project_base_path', '/var/aegir/projects') . '/' . $project_name, $form_state);
  }

  // If custom base URLs are not allowed, set the value here using the project name.
  if (!variable_get('devshop_projects_allow_custom_base_url', FALSE)) {
    form_set_value($form['base_url'], devshop_projects_url($project_name), $form_state);
  }
}

/**
 * Implements hook_form_alter().
 */
function devshop_projects_form_alter(&$form, &$form_state, $form_id) {

  // Removing unneccesary fieldgroups
  if ($form_id == 'project_node_form') {
    global $user;

    $project = $form['#node'];
    unset($form['menu']);
    unset($form['revision_information']);
    unset($form['author']);
    unset($form['options']);
    unset($form['actions']['delete']);
    unset($form['actions']['preview']);
    if (isset($form['retry']['#value'])) {
      $form['actions']['submit']['#value'] = t('Save and Retry');
    }

    //Add button for delete project
    $form['actions']['delete'] = array(
      '#markup' => l(t('Delete this project'), 'node/' . $project->nid . '/project_delete', array('query' => array('token' => drupal_get_token($user->uid)))),
      '#weight' => 10,
    );
  }

  // Create Project Wizard
  if (
    $form_id == 'devshop_project_create_step_git' ||
    $form_id == 'devshop_project_create_step_sites' ||
    $form_id == 'devshop_project_create_step_settings' ||
    $form_id == 'devshop_project_create_step_environments'
  ){
    if (isset($form['#no_finish'])) {
      unset($form['buttons']['return']);
    }
    if (isset($form['#no_next'])) {
      unset($form['buttons']['next']);
    }
    $form['buttons']['next']['#attributes'] =
    $form['buttons']['return']['#attributes'] = array(
      'class' => array(
          'btn btn-success',
      ),
    );
    $form['buttons']['previous']['#attributes'] = array(
      'class' => array(
          'btn btn-default',
      ),
    );
    $form['buttons']['cancel']['#attributes'] = array(
      'class' => array(
          'btn btn-link',
      ),
    );

  }

  // Hosting Task Forms
  if ($form_id == 'hosting_task_confirm_form') {
    switch ($form['task']['#value']) {

      // Create Environment form.
      case 'devshop-create':

        if (arg(3) == 'clone') {
          drupal_set_title(t('Clone environment'));
        }
        elseif (arg(3) == 'fork') {
          drupal_set_title(t('Fork environment'));
        }
        else {
          drupal_set_title(t('Create new environment'));
        }
        $form['#submit'] = array('hosting_task_devshop_create_form_submit');
        break;

      // Migrate Form: used for changing database server.
      case 'migrate':

        // To change the database server, we use the migrate form.
        if ($_GET['deploy'] == 'stack') {
          drupal_set_title(t('Change Database Server'));
          $site_node = node_load($form['nid']['#value']);
          $environment = $site_node->environment;
          $form['help']['#weight'] = 100;
          $form['help']['#value'] = t("Are you sure you want to change this environment's database server?");

          // Set URI value
          $form['parameters']['new_uri']['#type'] = 'value';
          $form['parameters']['new_uri']['#value'] = $form['parameters']['new_uri']['#default_value'];

          // Display something helpful
          $form['old'] = array(
            '#type' => 'item',
            '#title' => t('Current Database Server'),
            '#value' => l($environment->servers['db']['name'], 'hosting/c/server_' . $environment->servers['db']['nid']),
            '#weight' => '-1',
          );

          foreach ($form['parameters'] as $key => &$element) {
            if (is_int($key)) {
              // Don't unset the target platform. Just hide it.
              if (isset($element['target_platform']['#default_value'])) {
                $form['parameters'][$key]['target_platform']['#type'] = 'value';
                $form['parameters'][$key]['target_platform']['#value'] = $form['parameters'][$key]['target_platform']['#default_value'];
              }
              else {
                unset($form['parameters'][$key]);
              }
            }
          }
          $form['parameters']['#suffix'] = '</div>';
        }
        break;

      // Deploy task form
      case 'devshop-deploy':

        // Alter title of deploy task.
        $node = node_load($form['nid']['#value']);
        drupal_set_title(t('Deploy code to Environment "@env"', array('@env' => $node->environment->name)), PASS_THROUGH);
        $form['actions']['cancel']['#value'] = l(t('Cancel'), "node/{$node->project->nid}");
        break;

      // Sync task form
      case 'sync':
        if (isset($_GET['source'])) {
          $node = node_load($form['nid']['#value']);

          if ($_GET['source'] == 'other') {
            $source = '';
          }
          else {
            $source = node_load($_GET['source']);
            if (isset($source->nid) && $source->type != 'site') {
              break;
            }
          }

          drupal_set_title(t('Deploy data to Environment "@env"', array('@env' => $node->environment->name)), PASS_THROUGH);
          $environment = $node->environment;

          $form['help'] = array(
            '#prefix' => '<div class="alert alert-warning" role="alert">',
            '#suffix' => '</div>',

            'target' => array(
              '#markup' => t('Target: ') . l($environment->url, $environment->url, array('attributes' => array('target' => '_blank'))) . ' <span><i class="fa fa-warning"></i> ' . t('Data will be destroyed.') . '</span>',
              '#prefix' => '<p class="text-danger">',
              '#suffix' => '</p>',
            ),
          );

          // If source is empty, ask for an alias.
          if (empty($source)) {
            $form['parameters']['source'] = array(
              '#type' => 'textfield',
              '#title' => t('Drush Alias for Source'),
              '#description' => t('Enter a full drush alias to use as the source. Make sure to include the "@" character at the beginning.'),
            );
            $form['parameters']['destination'] = array(
              '#type' => 'value',
              '#value' => $node->environment->drush_alias,
            );
          }
          // If a source is specified, add help and set it as a value.
          else {

            $source_environment = $source->environment;
            $form['help']['source'] = array(
              '#markup' => t('Source: ') . l($source_environment->url, $source_environment->url, array('attributes' => array('target' => '_blank'))),
              '#prefix' => '<p>',
              '#suffix' => '</p>',
            );

            $form['parameters']['source'] = array(
              '#type' => 'value',
              '#value' => $source->environment->drush_alias,
            );
            $form['parameters']['destination'] = array(
              '#type' => 'value',
              '#value' => $node->environment->drush_alias,
            );
          }

          // Don't copy modules and themes.
          unset($form['parameters']['modules']);
          unset($form['parameters']['themes']);
          unset($form['parameters']['libraries']);

          // Better output
          $form['parameters']['database']['#prefix'] = '<label>' . t('What to Deploy?') . '</label>';
          $form['parameters']['registry-rebuild']['#prefix'] = '<label>' . t('Run Deploy Hooks?') . '</label>';
          $form['parameters']['backup']['#weight'] = -11;

          $form['actions']['submit']['#value'] = t('Deploy Data');
          $form['actions']['cancel']['#value'] = l(t('Cancel'), "node/{$node->project->nid}");

          $form['warning']['#markup'] = '<div class="alert alert-spacing alert-danger">' . t('Clicking "Deploy Data" will DESTROY the database for the environment !link.', array('!link' => l($environment->url, $environment->url, array('attributes' => array('target' => '_blank'))))) . '</div>';
        }
        break;
      case 'delete':
        $node = node_load($form['nid']['#value']);
        if ($node->type == 'project') {

          // Set a better title.
          drupal_set_title(t('Delete Project: !project', array(
            '!project' => l($node->project->name, "node/{$node->nid}"),
          )));

          // Get a list of environments.
          foreach ($node->project->environments as $environment) {
            $items[] = l($environment->uri, 'http://' . $environment->uri);

          }
          $environments = theme('item_list', array('items' => $items));

          // Display a scary warning message.
          $form['help']['#markup'] = '<div class="alert alert-spacing alert-danger">' . t('Are you sure you wish to destroy the project !project and all of these environments?', array(
              '!project' => l($node->project->name, "node/{$node->nid}"),
            )) . $environments . '</div>';

          // Make the button big
          $form['actions']['submit']['#value'] = t('Delete Project & Environments');
          $form['actions']['submit']['#attributes']['class'] = 'btn btn-large btn-danger';

        }
        break;
    }
  }

  // Platform "Edit" page.
  if ($form_id == 'platform_node_form') {

    // Set to values
    $platform_node = node_load($form['nid']['#value']);
    if (empty($platform_node->project)) {
      return;
    }

    $form['title']['#type'] = 'value';
    $form['makefile']['#type'] = 'value';
    unset($form['info']);

    // If switching webservers...
    if ($_GET['web_server']) {
      $environment = $platform_node->environment;
      $form['help'] = array(
        '#markup' => t("Are you sure you want to change this site's web server?  <strong>NOTE: You will have to change DNS as well!</strong>"),
        '#weight' => -10,
      );

      $web_server = $_GET['web_server'];
      $web_server_nid = db_query('SELECT nid FROM {node} WHERE type = :type && title = :title', array(':type' => "server", ':title' => $web_server))->fetchField();

      // Set values
      $form['web_server']['#default_value'] = $web_server_nid;
      $form['web_server']['#type'] = 'hidden';

      // Load servers to display IP addresses.
      $old_server = node_load($environment->servers['http']['nid']);
      $new_server = node_load($web_server_nid);

      // Display something helpful
      $form['old'] = array(
        '#type' => 'item',
        '#title' => t('Current Web Server'),
        '#markup' => l($environment->servers['http']['name'], 'node/' . $environment->servers['http']['nid']) . '<br />' . implode($old_server->ip_addresses, '<br />'),
        '#weight' => '-1',
      );

      // Display something helpful
      $form['new'] = array(
        '#type' => 'item',
        '#title' => t('New Web Server'),
        '#markup' => l($web_server, "node/$web_server_nid") . '<br />' . implode($new_server->ip_addresses, '<br />'),
        '#weight' => '0',
      );
    }
  }
}

/**
 * Implements hook_form_FORM_ID_alter() for node_site_form
 *
 * "Environment" Settings form.
 */
function devshop_projects_form_site_node_form_alter(&$form, &$form_state, $form_id) {

  // Get the node object for this form.
  $node = $form['#node'];

  // Look for #project (like on node create form)
  if ($node->project) {
    $project = $node->project;
  } elseif (isset($form['#project'])) {
    $project = $form['#project'];
  }
  else {
    return;
  }

  // If this site is in a project and we are on it's node/%/edit page, send to special environments form.
  if (!empty($project) && arg(1) == $node->nid) {
    drupal_goto("node/{$project->nid}/edit/env/{$node->environment->name}");
    exit;
  }

  // Get environment object.  (Convert if it is an array. This happens when clicking "Add an Alias".
  $environment = (object) $node->environment;

  // Remove aegir's helpful info panel.
  unset($form['info']);

  // Help Text
  $form['note'] = array(
      '#markup' => t('These settings apply only to the environment !site', array('!site' => l($environment->name, $environment->url))),      '#weight' => -10,
    );

  $form['environment'] = array(
      '#weight' => -9,
      '#tree' => true,
  );

  // Values
  $form['environment']['project_nid'] = array(
      '#value' => $environment->project_nid,
      '#type' => 'value',
  );
  $form['environment']['project_name'] = array(
      '#value' => $environment->project_name,
      '#type' => 'value',
  );
  $form['environment']['name'] = array(
      '#value' => $environment->name,
      '#type' => 'value',
  );
  $form['environment']['git_ref'] = array(
      '#value' => $environment->git_ref,
      '#type' => 'value',
  );
  $form['environment']['site'] = array(
      '#value' => $environment->site,
      '#type' => 'value',
  );
  $form['environment']['platform'] = array(
      '#value' => $environment->platform,
      '#type' => 'value',
  );

  // User-configurable Settings
  $form['environment']['settings'] = array(
      '#type' => 'fieldset',
      '#title' => t('Environment Settings'),
      '#tree' => true,
  );
  $form['environment']['settings']['locked'] = array(
      '#type' => 'checkbox',
      '#title' => t('Lock Database'),
      '#default_value' => isset($environment->settings->locked) ? $environment->settings->locked : FALSE,
      '#description' => t('Prevent devshop users from destroying the database.') . '<p class="text-danger">Drush users may still overwrite the database.</p>',
  );

  $form['environment']['settings']['pull_disabled'] = array(
      '#type' => 'checkbox',
      '#title' => t('Disable Deploy on Commit'),
      '#default_value' => isset($environment->settings->pull_disabled) ? $environment->settings->pull_disabled : FALSE,
      '#description' => t('Do not pull code to the server on commit & push.'),
  );

  // Load deploy hooks form element.
  $form['environment']['settings']['deploy'] = devshop_environment_deploy_hooks_form($project, $environment);

  $form['environment']['last_task'] = array(
    '#type' => 'value',
    '#value' => $environment->last_task,
  );

  // Add our own submit handler
  $form['#submit'][] = 'devshop_projects_environment_settings_submit';
}

/**
 * Helper to output the forms selection for Deploy Hooks.
 *
 * Used in the Projects settings form, environment settings form, deploy task form, and sync form.
 *
 * @param $project
 * @param null $environment
 * @return array
 */
function devshop_environment_deploy_hooks_form($project, $environment = NULL, $task_type = NULL) {

  // If environment not specified, just grab one.
  if (!$environment) {
    $environment = current($project->environments);
    $is_environment_form = FALSE;
  }
  else {
    $is_environment_form = TRUE;
  }
  $return = array(
      '#type' => 'fieldset',
      '#group' => 'project_settings',
      '#title' => t('Deployment Hooks'),
      '#description' => t('Deployment hooks are run whenever your codebase changes. It is recommended to always enable database updates and cache clearing.'),
      '#weight' => -9,
  );

  // If we are on the project creation wizard, set some sane defaults
  if (current_path() == 'projects/add/settings') {
    $environment_update = FALSE;
    $environment_cache = FALSE;
    $environment_revert = FALSE;
    $environment_dothooks = FALSE;
    $environment_acquia_hooks  = FALSE;
    $project_update = TRUE;
    $project_cache  = TRUE;
    $project_revert = FALSE;
    $project_dothooks = FALSE;
    $project_acquia_hooks = FALSE;
  }
  else {
    $environment_update = isset($environment->settings->deploy) ? $environment->settings->deploy['update'] : FALSE;
    $environment_cache = isset($environment->settings->deploy) ? $environment->settings->deploy['cache'] : FALSE;
    $environment_revert = isset($environment->settings->deploy) ? $environment->settings->deploy['revert'] : FALSE;
    $environment_dothooks = isset($environment->settings->deploy) ? $environment->settings->deploy['dothooks'] : FALSE;
    $environment_acquia_hooks = isset($environment->settings->deploy) ? $environment->settings->deploy['acquia_hooks'] : FALSE;
    $project_update = isset($project->settings->deploy) ? $project->settings->deploy['default_hooks']['update'] : FALSE;
    $project_cache = isset($project->settings->deploy) ? $project->settings->deploy['default_hooks']['cache'] : FALSE;
    $project_revert = isset($project->settings->deploy) ? $project->settings->deploy['default_hooks']['revert'] : FALSE;
    $project_dothooks = isset($project->settings->deploy) ? $project->settings->deploy['default_hooks']['dothooks'] : FALSE;
    $project_acquia_hooks = isset($project->settings->deploy) ? $project->settings->deploy['default_hooks']['acquia_hooks'] : FALSE;
  }

  $return['update'] = array(
      '#type' => 'checkbox',
      '#title' => t('Run database updates.'),
      '#default_value' => empty($environment->settings->deploy) ? $project_update : $environment_update,
  );
  $return['cache'] = array(
      '#type' => 'checkbox',
      '#title' => t('Clear all caches.'),
      '#default_value' => empty($environment->settings->deploy)? $project_cache : $environment_cache,
  );
  $return['revert'] = array(
    '#type' => 'checkbox',
    '#title' => t('Revert all features.'),
    '#description' => t('If features is enabled, revert all of them.'),
    '#default_value' => empty($environment->settings->deploy)? $project_revert : $environment_revert,
  );

  // Look for .hooks or .hooks.yml
  if (!$is_environment_form || isset($project->settings->deploy, $project->settings->deploy['default_hooks']['dothooks'])) {
    $return['dothooks'] = array(
      '#type' => 'checkbox',
      '#title' => t('Run <code>deploy</code> commands in the <code>.hooks</code> file.'),
      '#default_value' => empty($environment->settings->deploy) ? $project_dothooks : $environment_dothooks,
      '#description' => t('You can add your desired deploy hooks to a file in the root folder of your project. This is recommended as it gives your developers control over what happens when their code is deployed. See the !dothooks for more information. <em>If you use a <code>.hooks</code> file you probably want to uncheck the deploy hooks here.</em>', array(
        '!dothooks' => l(t('.hooks documentation'), 'admin/help/devshop_dothooks'),
      )),
    );
  }

  // Look for acquia cloud hooks.
  if (!$is_environment_form || isset($project->settings->deploy, $project->settings->deploy['default_hooks']['acquia_hooks'])) {
    $return['acquia_hooks'] = array(
      '#type' => 'checkbox',
      '#title' => t('Run Acquia Cloud Hooks'),
      '#description' => '<p>' . t('Acquia Cloud Hooks were detected in your project.  Check this box to run all acquia cloud hooks.') . '</p><p>' . t('Available cloud hooks are: "post-code-update" when deploying code via git, "post-code-deploy" when deploying code manually, "post-db-copy" when running a sync task with a database, and "post-files-copy" when running a sync task with files.') . '</p>',
      '#default_value' => empty($environment->settings->deploy) ? $project_acquia_hooks : $environment_acquia_hooks,
    );
  }

  // Disable deploy hooks on environment form, deploy task form ,and sync task form.
  if (
    // Environment form
    ($is_environment_form && !$project->settings->deploy['allow_environment_deploy_config']) ||
    ($task_type == 'devshop-deploy' && !$project->settings->deploy['allow_environment_deploy_hooks_override'])
  ) {

    foreach (element_children($return) as $i) {
      $return[$i]['#disabled'] = TRUE;
    }

    $return['#suffix'] = t('Environment-specific deploy hook configuration is not allowed. Check Project Settings to change.');
  }
  return $return;
}

/**
 * Submit handler for site/environment settings page.
 */
function devshop_projects_environment_settings_submit($form, &$form_state) {

  // Save the environment record.
  // @TODO: Replace with an $environment->save();

  // Ensure correct data types
  $environment = (object) $form_state['values']['environment'];
  $environment->settings = (array) $environment->settings;

  // Prepare record for saving
  $info = new stdClass();
  $info->project_nid = $environment->project_nid;
  $info->name = $environment->name;
  $info->git_ref = $environment->git_ref;
  $info->site = $environment->site;
  $info->platform = $environment->platform;
  $info->settings = serialize($environment->settings);

  // Save environment record.
  if (drupal_write_record('hosting_devshop_project_environment', $info, array('project_nid', 'name'))) {

    // Remove the "Site dev.drupal.devshop.local has been updated.
    drupal_get_messages('status');

    // Send our own message
    drupal_set_message(t('Environment settings saved for %env in project %project', array('%env' => $environment->name, '%project' => $environment->project_name)));
  }
  else {
    watchdog('error', 'Environment record not saved: ' . print_r($info, 1));
  }

  $form_state['redirect'] = 'node/' . $environment->project_nid;
}


/**
 * Implements hook_form_alter().
 */
function devshop_projects_form_server_node_form_alter(&$form, &$form_state, $form_id) {

  $items = array();
  $items[] = t('To create a new server, you must first connect it to this one via SSH.');
  $items[] = t("You must grant %aegir access to your new server by adding it's SSH Key (below) to <code>~/.ssh/authorized_keys</code>.", array(
    '%aegir' => "aegir@" . $_SERVER['HTTP_HOST'],
  ));
  $items[] = t('If you wish to provision a new server, add the key to the
<code>root</code> user, then run the <code>devshop remote:install</code> command.');
  $items[] = t('If you have already provisioned the server, add the key to the <code>aegir</code> user.');

  $form['note'] = array(
    '#markup' => theme('item_list', array('items' => $items)),
    '#weight' => -100,
    '#prefix' => '<div class="alert alert-info">',
    '#suffix' => '</div>',
  );

  $form['key'] = array(
    '#type' => 'item',
    '#title' => t(' SSH Public Key for %aegir', array(
      '%aegir' => "aegir@" . $_SERVER['HTTP_HOST'],
    )),
    '#markup' => '<pre>' . variable_get('devshop_public_key', '') . '</pre>',
    '#weight' => -99,
  );
}

function devshop_projects_form_hosting_task_update_status_form_alter(&$form, &$form_state, $form_id){

  $form['update-status']['#value'] = t('Cancel Task');
  $form['update-status']['#attributes']['class'] = 'btn-link pull-right';
}
