<?php

namespace Drupal\page_load_progress\Form;

use Drupal\Core\Form\ConfigFormBase;
use Drupal\Core\Form\FormStateInterface;

/**
 * Allows to configure the page_load_progress module.
 */
class SettingsForm extends ConfigFormBase {

  /**
   * {@inheritdoc}
   */
  public function getFormId() {
    return 'page_load_progress_admin_form';
  }

  /**
   * {@inheritdoc}
   */
  protected function getEditableConfigNames() {
    return [
      'page_load_progress.settings',
    ];
  }

  /**
   * {@inheritdoc}
   */
  public function buildForm(array $form, FormStateInterface $form_state) {
    $page_load_progress_config = $this->config('page_load_progress.settings');

    $form['page_load_progress_time'] = [
      '#type' => 'select',
      '#title' => $this->t('Time to wait before locking the screen'),
      '#description' => $this->t('Enter the time you want to wait before showing the image lock.'),
      '#options' => [
        10   => $this->t('Show immediately'),
        1000 => $this->t('Show after a 1 second delay'),
        3000 => $this->t('Show after a 3 seconds delay'),
        5000 => $this->t('Show after a 5 seconds delay'),
      ],
      '#default_value' => $page_load_progress_config->get('page_load_progress_time'),
    ];

    $form['page_load_progress_elements'] = [
      '#type' => 'textfield',
      '#title' => $this->t('Elements that will trigger the throbber'),
      '#description' => $this->t('Enter the elements that will trigger the effect separated by commas. Classes should have their leading dot and ids their leading hashes.'),
      '#required' => TRUE,
      '#default_value' => $page_load_progress_config->get('page_load_progress_elements'),
    ];

    return parent::buildForm($form, $form_state);
  }

  /**
   * {@inheritdoc}
   */
  public function submitForm(array &$form, FormStateInterface $form_state) {
    $this->config('page_load_progress.settings')
      ->set('page_load_progress_time', $form_state->getValue('page_load_progress_time'))
      ->set('page_load_progress_elements', $form_state->getValue('page_load_progress_elements'))
      ->save();

    parent::submitForm($form, $form_state);
  }

}
